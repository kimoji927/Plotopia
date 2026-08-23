// CleanupBpPrintsCommandlet.cpp
#include "Commandlets/CleanupBpPrintsCommandlet.h"

#if WITH_EDITOR

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "K2Node_CallFunction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

static int32 GTotalRemoved = 0;

/** 结构节点（事件/函数出入口/注释/自引用/宏/数据表行节点等）永远保留 */
static bool IsStructuralNode(UEdGraphNode* Node)
{
	const FString Name = Node->GetClass()->GetName();
	return Name.StartsWith(TEXT("K2Node_Event"))
		|| Name.StartsWith(TEXT("K2Node_FunctionEntry"))
		|| Name.StartsWith(TEXT("K2Node_FunctionResult"))
		|| Name == TEXT("EdGraphNode_Comment")
		|| Name == TEXT("K2Node_Self")
		|| Name == TEXT("K2Node_MacroInstance")
		|| Name == TEXT("K2Node_GetDataTableRow")
		|| Name == TEXT("K2Node_VariableSet")
		|| Name == TEXT("K2Node_GetDataTableRow");
}

static bool IsDebugPrintNode(UEdGraphNode* Node)
{
	UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node);
	if (!Call) return false;
	const FName MemberName = Call->FunctionReference.GetMemberName();
	return MemberName == TEXT("PrintString")
		|| MemberName == TEXT("PrintText")
		|| MemberName == TEXT("PrintWarning")
		|| MemberName == TEXT("PrintError");
}

/**
 * 删除图中的调试打印节点：
 * 1) 打印节点前后执行线直接连通，保证原执行流不断；
 * 2) 删除打印后，把完全失去连接的孤儿辅助节点一并清理。
 */
static void CleanupGraph(UBlueprint* Blueprint, UEdGraph* Graph)
{
	if (!Graph || !Blueprint) return;

	// ---- 第一遍：删除打印节点并重连执行流 ----
	TArray<UK2Node_CallFunction*> PrintNodes;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (IsDebugPrintNode(Node))
		{
			PrintNodes.Add(CastChecked<UK2Node_CallFunction>(Node));
		}
	}

	for (UK2Node_CallFunction* Print : PrintNodes)
	{
		UEdGraphPin* ExecIn = Print->FindPin(UEdGraphSchema_K2::PN_Execute, EGPD_Input);
		UEdGraphPin* ExecOut = Print->FindPin(UEdGraphSchema_K2::PN_Then, EGPD_Output);

		// 收集打印节点的上游执行源与下游执行目标
		UEdGraphPin* SourcePin = (ExecIn && ExecIn->LinkedTo.Num() > 0) ? ExecIn->LinkedTo[0] : nullptr;
		TArray<UEdGraphPin*> TargetPins;
		if (ExecOut)
		{
			for (UEdGraphPin* T : ExecOut->LinkedTo)
			{
				TargetPins.Add(T);
			}
		}

		if (ExecIn) ExecIn->BreakAllPinLinks();
		if (ExecOut) ExecOut->BreakAllPinLinks();

		// 上游直接接下游，保持执行流不断
		if (SourcePin)
		{
			for (UEdGraphPin* T : TargetPins)
			{
				SourcePin->MakeLinkTo(T);
			}
		}

		FBlueprintEditorUtils::RemoveNode(Blueprint, Print, true);
		++GTotalRemoved;
	}

	// ---- 第二遍：清理完全断线的孤儿节点（多轮直到无变化） ----
	for (int32 Round = 0; Round < 20; ++Round)
	{
		TArray<UEdGraphNode*> Orphans;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (IsStructuralNode(Node)) continue;
			bool bAnyLink = false;
			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin && Pin->LinkedTo.Num() > 0)
				{
					bAnyLink = true;
					break;
				}
			}
			if (!bAnyLink)
			{
				Orphans.Add(Node);
			}
		}
		if (Orphans.Num() == 0) break;
		for (UEdGraphNode* Node : Orphans)
		{
			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin) Pin->BreakAllPinLinks();
			}
			FBlueprintEditorUtils::RemoveNode(Blueprint, Node, true);
			++GTotalRemoved;
		}
	}
}

static void ProcessBlueprint(const FString& AssetPath)
{
	UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *AssetPath);
	if (!BP)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CleanupBpPrints] 未找到蓝图 %s"), *AssetPath);
		return;
	}

	const int32 Before = GTotalRemoved;
	for (UEdGraph* G : BP->UbergraphPages) CleanupGraph(BP, G);
	for (UEdGraph* G : BP->FunctionGraphs) CleanupGraph(BP, G);
	for (UEdGraph* G : BP->MacroGraphs) CleanupGraph(BP, G);

	const int32 Removed = GTotalRemoved - Before;
	if (Removed > 0)
	{
		BP->Modify();
		// 重新编译，使删除的节点真正从生成的类中移除
		FKismetEditorUtilities::CompileBlueprint(BP);

		// 显式保存资产并记录结果
		UPackage* Pkg = BP->GetOutermost();
		if (Pkg)
		{
			Pkg->SetDirtyFlag(true);
			FString FileName = FPackageName::LongPackageNameToFilename(Pkg->GetName(), FPackageName::GetAssetPackageExtension());
			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = RF_Standalone;
			SaveArgs.SaveFlags = SAVE_NoError;
			const bool bSaved = UPackage::SavePackage(Pkg, BP, *FileName, SaveArgs);
			UE_LOG(LogTemp, Log, TEXT("[CleanupBpPrints] %s : 移除 %d 个节点，保存%s (%s)"),
				*AssetPath, Removed, bSaved ? TEXT("成功") : TEXT("失败"), *FileName);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[CleanupBpPrints] %s : 无法获取资产包"), *AssetPath);
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[CleanupBpPrints] %s : 无可移除的调试节点"), *AssetPath);
	}
}

#endif // WITH_EDITOR

int32 UCleanupBpPrintsCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	UE_LOG(LogTemp, Log, TEXT("=== CleanupBpPrintsCommandlet started ==="));

	static const TArray<FString> Assets = {
		TEXT("/Game/GAS/UI/Inventory/WBP_ItemSlot"),
		TEXT("/Game/GAS/UI/Inventory/WBP_Inventory"),
		TEXT("/Game/GAS/UI/Inventory/WBP_Hotbar"),
		TEXT("/Game/GAS/UI/Inventory/WBP_DragVisual"),
		TEXT("/Game/GAS/UI/Inventory/WBP_DropZone"),
		TEXT("/Game/GAS/UI/HUD/WBP_Inv_HUDWidget"),
		TEXT("/Game/GAS/UI/HUD/WBP_Inv_PickupMessage"),
		TEXT("/Game/GAS/UI/Startup/WBP_MainMeun"),
		TEXT("/Game/GAS/UI/Startup/WBP_GameMenu"),
		TEXT("/Game/GAS/UI/Startup/Base/WBP_ButtonBase"),
		TEXT("/Game/GAS/UI/Startup/Base/WBP_MainButtonBase"),
		TEXT("/Game/GAS/UI/Startup/Base/WBP_ServerBrowser"),
	};
	for (const FString& Asset : Assets)
	{
		ProcessBlueprint(Asset);
	}

	UE_LOG(LogTemp, Log, TEXT("=== CleanupBpPrintsCommandlet finished: 共移除 %d 个节点 ==="), GTotalRemoved);
	return 0;
#else
	return 0;
#endif
}
