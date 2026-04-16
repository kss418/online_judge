import { computed } from 'vue'

export function useProblemEditorSections({
  isLoadingDetail,
  detailErrorMessage,
  selectedProblemDetail,
  busySection,
  canSaveTitle,
  canSaveLimits,
  canSaveStatement,
  canCreateSample,
  canUploadTestcaseZip,
  canDeleteLastSample,
  isSavingTitle,
  isSavingLimits,
  isSavingStatement,
  isCreatingSample,
  isDeletingLastSample,
  isUploadingTestcaseZip,
  titleDraft,
  timeLimitDraft,
  memoryLimitDraft,
  descriptionDraft,
  inputFormatDraft,
  outputFormatDraft,
  noteDraft,
  sampleDrafts,
  testcaseZipInputKey,
  selectedTestcaseZipName,
  formatCount,
  isSavingSample,
  canSaveSample,
  isLastSample,
  handleSaveTitle,
  handleSaveLimits,
  handleSaveStatement,
  handleCreateSample,
  handleSaveSample,
  handleDeleteLastSample,
  handleTestcaseZipFileChange,
  handleUploadTestcaseZip,
  openRejudgeDialog,
  openDeleteDialog
}){
  const actions = {
    updateTitleDraft(value){
      titleDraft.value = value
    },
    updateTimeLimitDraft(value){
      timeLimitDraft.value = value
    },
    updateMemoryLimitDraft(value){
      memoryLimitDraft.value = value
    },
    updateDescriptionDraft(value){
      descriptionDraft.value = value
    },
    updateInputFormatDraft(value){
      inputFormatDraft.value = value
    },
    updateOutputFormatDraft(value){
      outputFormatDraft.value = value
    },
    updateNoteDraft(value){
      noteDraft.value = value
    },
    saveTitle: handleSaveTitle,
    saveLimits: handleSaveLimits,
    saveStatement: handleSaveStatement,
    createSample: handleCreateSample,
    saveSample: handleSaveSample,
    deleteLastSample: handleDeleteLastSample,
    changeTestcaseZip: handleTestcaseZipFileChange,
    uploadTestcaseZip: handleUploadTestcaseZip,
    openRejudgeDialog,
    openDeleteDialog
  }

  return computed(() => {
    const shell = isLoadingDetail.value
      ? {
        mode: 'loading',
        message: '문제 정보를 불러오는 중입니다.'
      }
      : detailErrorMessage.value
        ? {
          mode: 'error',
          message: detailErrorMessage.value
        }
        : !selectedProblemDetail.value
          ? {
            mode: 'empty',
            message: '왼쪽 목록에서 문제를 선택하거나 새 문제를 생성하세요.'
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
            selectedProblemDetail: selectedProblemDetail.value,
            busySection: busySection.value,
            formatCount
          },
          actions: {
            openRejudgeDialog: actions.openRejudgeDialog
          }
        },
        basics: {
          model: {
            titleDraft: titleDraft.value,
            timeLimitDraft: timeLimitDraft.value,
            memoryLimitDraft: memoryLimitDraft.value,
            canSaveTitle: canSaveTitle.value,
            canSaveLimits: canSaveLimits.value,
            isSavingTitle: isSavingTitle.value,
            isSavingLimits: isSavingLimits.value
          },
          actions: {
            updateTitleDraft: actions.updateTitleDraft,
            updateTimeLimitDraft: actions.updateTimeLimitDraft,
            updateMemoryLimitDraft: actions.updateMemoryLimitDraft,
            saveTitle: actions.saveTitle,
            saveLimits: actions.saveLimits
          }
        },
        statement: {
          model: {
            descriptionDraft: descriptionDraft.value,
            inputFormatDraft: inputFormatDraft.value,
            outputFormatDraft: outputFormatDraft.value,
            noteDraft: noteDraft.value,
            canSaveStatement: canSaveStatement.value,
            isSavingStatement: isSavingStatement.value
          },
          actions: {
            updateDescriptionDraft: actions.updateDescriptionDraft,
            updateInputFormatDraft: actions.updateInputFormatDraft,
            updateOutputFormatDraft: actions.updateOutputFormatDraft,
            updateNoteDraft: actions.updateNoteDraft,
            saveStatement: actions.saveStatement
          }
        },
        samples: {
          model: {
            sampleDrafts: sampleDrafts.value,
            canCreateSample: canCreateSample.value,
            canDeleteLastSample: canDeleteLastSample.value,
            isCreatingSample: isCreatingSample.value,
            isDeletingLastSample: isDeletingLastSample.value,
            formatCount,
            isSavingSample,
            canSaveSample,
            isLastSample
          },
          actions: {
            createSample: actions.createSample,
            saveSample: actions.saveSample,
            deleteLastSample: actions.deleteLastSample
          }
        },
        testcaseUpload: {
          model: {
            testcaseZipInputKey: testcaseZipInputKey.value,
            selectedTestcaseZipName: selectedTestcaseZipName.value,
            busySection: busySection.value,
            canUploadTestcaseZip: canUploadTestcaseZip.value,
            isUploadingTestcaseZip: isUploadingTestcaseZip.value
          },
          actions: {
            changeTestcaseZip: actions.changeTestcaseZip,
            uploadTestcaseZip: actions.uploadTestcaseZip
          }
        },
        danger: {
          model: {
            busySection: busySection.value
          },
          actions: {
            openDeleteDialog: actions.openDeleteDialog
          }
        }
      }
    }
  })
}
