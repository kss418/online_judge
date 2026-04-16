import { computed } from 'vue'

export function useTestcaseEditorSections({
  selectedProblemId,
  isLoadingProblem,
  problemErrorMessage,
  problemDetail,
  busySection,
  canUploadTestcaseZip,
  isUploadingTestcaseZip,
  canCreateTestcase,
  isCreatingTestcase,
  canViewSpecificTestcase,
  isLoadingTestcases,
  isLoadingSelectedTestcase,
  testcaseItems,
  testcaseErrorMessage,
  selectedTestcaseErrorMessage,
  selectedTestcaseOrder,
  selectedTestcase,
  canDeleteSelectedTestcase,
  isDeletingSelectedTestcase,
  canMoveTestcases,
  isMovingTestcase,
  canSaveSelectedTestcase,
  isSavingSelectedTestcase,
  formatCount,
  describeTestcaseContent,
  isLastTestcase,
  setTestcaseSummaryElement,
  testcaseZipInputKey,
  selectedTestcaseZipName,
  newTestcaseInput,
  newTestcaseOutput,
  viewTestcaseOrderInput,
  selectedTestcaseInputDraft,
  selectedTestcaseOutputDraft,
  handleTestcaseZipFileChange,
  handleUploadTestcaseZip,
  handleCreateTestcase,
  selectTestcase,
  handleDeleteSelectedTestcase,
  handleMoveTestcase,
  handleSaveSelectedTestcase,
  handleViewSelectedTestcase
}){
  const actions = {
    updateNewTestcaseInput(value){
      newTestcaseInput.value = value
    },
    updateNewTestcaseOutput(value){
      newTestcaseOutput.value = value
    },
    updateViewTestcaseOrderInput(value){
      viewTestcaseOrderInput.value = value
    },
    updateSelectedTestcaseInputDraft(value){
      selectedTestcaseInputDraft.value = value
    },
    updateSelectedTestcaseOutputDraft(value){
      selectedTestcaseOutputDraft.value = value
    },
    changeTestcaseZip: handleTestcaseZipFileChange,
    uploadTestcaseZip: handleUploadTestcaseZip,
    createTestcase: handleCreateTestcase,
    viewSelectedTestcase: handleViewSelectedTestcase,
    selectTestcase,
    deleteSelectedTestcase: handleDeleteSelectedTestcase,
    moveTestcase: handleMoveTestcase,
    saveSelectedTestcase: handleSaveSelectedTestcase
  }

  return computed(() => {
    const shell = !selectedProblemId.value
      ? {
        mode: 'empty',
        message: '왼쪽 목록에서 문제를 선택하면 테스트케이스를 관리할 수 있습니다.'
      }
      : isLoadingProblem.value
        ? {
          mode: 'loading',
          message: '문제 정보를 불러오는 중입니다.'
        }
        : problemErrorMessage.value
          ? {
            mode: 'error',
            message: problemErrorMessage.value
          }
          : !problemDetail.value
            ? {
              mode: 'empty',
              message: '문제 정보를 불러오지 못했습니다.'
            }
            : {
              mode: 'ready',
              message: ''
            }

    return {
      shell,
      sections: {
        header: {
          model: {
            problemDetail: problemDetail.value,
            testcaseItems: testcaseItems.value,
            formatCount
          },
          actions: {}
        },
        upload: {
          model: {
            testcaseZipInputKey: testcaseZipInputKey.value,
            busySection: busySection.value,
            selectedTestcaseZipName: selectedTestcaseZipName.value,
            canUploadTestcaseZip: canUploadTestcaseZip.value,
            isUploadingTestcaseZip: isUploadingTestcaseZip.value
          },
          actions: {
            changeTestcaseZip: actions.changeTestcaseZip,
            uploadTestcaseZip: actions.uploadTestcaseZip
          }
        },
        create: {
          model: {
            newTestcaseInput: newTestcaseInput.value,
            newTestcaseOutput: newTestcaseOutput.value,
            busySection: busySection.value,
            canCreateTestcase: canCreateTestcase.value,
            isCreatingTestcase: isCreatingTestcase.value
          },
          actions: {
            updateNewTestcaseInput: actions.updateNewTestcaseInput,
            updateNewTestcaseOutput: actions.updateNewTestcaseOutput,
            createTestcase: actions.createTestcase
          }
        },
        reorder: {
          model: {
            viewTestcaseOrderInput: viewTestcaseOrderInput.value,
            canViewSpecificTestcase: canViewSpecificTestcase.value,
            isLoadingTestcases: isLoadingTestcases.value,
            testcaseItems: testcaseItems.value,
            testcaseErrorMessage: testcaseErrorMessage.value,
            selectedTestcaseOrder: selectedTestcaseOrder.value,
            selectedTestcase: selectedTestcase.value,
            canMoveTestcases: canMoveTestcases.value,
            isMovingTestcase: isMovingTestcase.value,
            formatCount,
            describeTestcaseContent,
            isLastTestcase,
            setTestcaseSummaryElement
          },
          actions: {
            updateViewTestcaseOrderInput: actions.updateViewTestcaseOrderInput,
            viewSelectedTestcase: actions.viewSelectedTestcase,
            selectTestcase: actions.selectTestcase,
            moveTestcase: actions.moveTestcase
          }
        },
        detail: {
          model: {
            isLoadingTestcases: isLoadingTestcases.value,
            testcaseErrorMessage: testcaseErrorMessage.value,
            testcaseItems: testcaseItems.value,
            isLoadingSelectedTestcase: isLoadingSelectedTestcase.value,
            selectedTestcaseErrorMessage: selectedTestcaseErrorMessage.value,
            selectedTestcase: selectedTestcase.value,
            canDeleteSelectedTestcase: canDeleteSelectedTestcase.value,
            isDeletingSelectedTestcase: isDeletingSelectedTestcase.value,
            busySection: busySection.value,
            selectedTestcaseInputDraft: selectedTestcaseInputDraft.value,
            selectedTestcaseOutputDraft: selectedTestcaseOutputDraft.value,
            canSaveSelectedTestcase: canSaveSelectedTestcase.value,
            isSavingSelectedTestcase: isSavingSelectedTestcase.value,
            formatCount
          },
          actions: {
            deleteSelectedTestcase: actions.deleteSelectedTestcase,
            updateSelectedTestcaseInputDraft: actions.updateSelectedTestcaseInputDraft,
            updateSelectedTestcaseOutputDraft: actions.updateSelectedTestcaseOutputDraft,
            saveSelectedTestcase: actions.saveSelectedTestcase
          },
          metadataSection: {
            model: {
              selectedTestcase: selectedTestcase.value,
              canDeleteSelectedTestcase: canDeleteSelectedTestcase.value,
              isDeletingSelectedTestcase: isDeletingSelectedTestcase.value,
              formatCount
            },
            actions: {
              deleteSelectedTestcase: actions.deleteSelectedTestcase
            }
          }
        }
      }
    }
  })
}
