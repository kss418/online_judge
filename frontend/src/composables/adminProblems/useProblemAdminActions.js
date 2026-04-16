import { useProblemBasicsActions } from '@/composables/adminProblems/useProblemBasicsActions'
import { useProblemCrudActions } from '@/composables/adminProblems/useProblemCrudActions'
import { useProblemSampleActions } from '@/composables/adminProblems/useProblemSampleActions'
import { useProblemStatementActions } from '@/composables/adminProblems/useProblemStatementActions'
import { useProblemTestcaseZipActions } from '@/composables/adminProblems/useProblemTestcaseZipActions'

export function useProblemAdminActions({
  authState,
  formatCount,
  busySection,
  feedback,
  newProblemTitle,
  selectedProblemDetail,
  updateSelectedProblemDetail,
  testcaseZipFile,
  resetTestcaseZipSelection,
  titleDraft,
  timeLimitDraft,
  memoryLimitDraft,
  descriptionDraft,
  inputFormatDraft,
  outputFormatDraft,
  noteDraft,
  canSaveTitle,
  canSaveLimits,
  canSaveStatement,
  canSaveSample,
  getSampleDraft,
  syncSampleDrafts,
  setSelectedProblemSamples,
  applySelectedProblemVersion,
  mergeProblemSummary,
  loadProblems,
  loadSelectedProblem,
  onCreatedProblem
}){
  const crudActions = useProblemCrudActions({
    authState,
    busySection,
    formatCount,
    newProblemTitle,
    selectedProblemDetail,
    canDeleteSelectedProblem: feedback.canDeleteSelectedProblem,
    canRejudgeSelectedProblem: feedback.canRejudgeSelectedProblem,
    setActionFeedback: feedback.setActionFeedback,
    closeDeleteDialog: feedback.closeDeleteDialog,
    closeRejudgeDialog: feedback.closeRejudgeDialog,
    onCreatedProblem,
    loadProblems
  })
  const basicsActions = useProblemBasicsActions({
    authState,
    busySection,
    formatCount,
    selectedProblemDetail,
    updateSelectedProblemDetail,
    titleDraft,
    timeLimitDraft,
    memoryLimitDraft,
    canSaveTitle,
    canSaveLimits,
    applySelectedProblemVersion,
    mergeProblemSummary,
    setActionFeedback: feedback.setActionFeedback
  })
  const statementActions = useProblemStatementActions({
    authState,
    busySection,
    formatCount,
    selectedProblemDetail,
    updateSelectedProblemDetail,
    descriptionDraft,
    inputFormatDraft,
    outputFormatDraft,
    noteDraft,
    canSaveStatement,
    applySelectedProblemVersion,
    setActionFeedback: feedback.setActionFeedback
  })
  const sampleActions = useProblemSampleActions({
    authState,
    busySection,
    formatCount,
    selectedProblemDetail,
    canSaveSample,
    getSampleDraft,
    syncSampleDrafts,
    setSelectedProblemSamples,
    applySelectedProblemVersion,
    setActionFeedback: feedback.setActionFeedback
  })
  const testcaseZipActions = useProblemTestcaseZipActions({
    authState,
    busySection,
    formatCount,
    selectedProblemDetail,
    testcaseZipFile,
    resetTestcaseZipSelection,
    applySelectedProblemVersion,
    loadSelectedProblem,
    setActionFeedback: feedback.setActionFeedback
  })

  return {
    actionMessage: feedback.actionMessage,
    actionErrorMessage: feedback.actionErrorMessage,
    rejudgeDialogOpen: feedback.rejudgeDialogOpen,
    rejudgeConfirmProblemId: feedback.rejudgeConfirmProblemId,
    rejudgeConfirmTitle: feedback.rejudgeConfirmTitle,
    deleteDialogOpen: feedback.deleteDialogOpen,
    deleteConfirmProblemId: feedback.deleteConfirmProblemId,
    deleteConfirmTitle: feedback.deleteConfirmTitle,
    canCreateProblem: crudActions.canCreateProblem,
    isCreatingProblem: crudActions.isCreatingProblem,
    isSavingTitle: basicsActions.isSavingTitle,
    isSavingLimits: basicsActions.isSavingLimits,
    isSavingStatement: statementActions.isSavingStatement,
    isCreatingSample: sampleActions.isCreatingSample,
    isDeletingLastSample: sampleActions.isDeletingLastSample,
    isUploadingTestcaseZip: testcaseZipActions.isUploadingTestcaseZip,
    isRejudgingProblem: crudActions.isRejudgingProblem,
    isDeletingProblem: crudActions.isDeletingProblem,
    canCreateSample: sampleActions.canCreateSample,
    canUploadTestcaseZip: testcaseZipActions.canUploadTestcaseZip,
    canDeleteLastSample: sampleActions.canDeleteLastSample,
    canDeleteSelectedProblem: feedback.canDeleteSelectedProblem,
    canRejudgeSelectedProblem: feedback.canRejudgeSelectedProblem,
    clearActionError: feedback.clearActionError,
    setActionFeedback: feedback.setActionFeedback,
    resetActionState: feedback.resetActionState,
    handleCreateProblem: crudActions.handleCreateProblem,
    handleSaveTitle: basicsActions.handleSaveTitle,
    handleSaveLimits: basicsActions.handleSaveLimits,
    handleSaveStatement: statementActions.handleSaveStatement,
    handleCreateSample: sampleActions.handleCreateSample,
    handleSaveSample: sampleActions.handleSaveSample,
    handleDeleteLastSample: sampleActions.handleDeleteLastSample,
    handleUploadTestcaseZip: testcaseZipActions.handleUploadTestcaseZip,
    openDeleteDialog: feedback.openDeleteDialog,
    openRejudgeDialog: feedback.openRejudgeDialog,
    closeDeleteDialog: feedback.closeDeleteDialog,
    closeRejudgeDialog: feedback.closeRejudgeDialog,
    handleRejudgeProblem: crudActions.handleRejudgeProblem,
    handleDeleteProblem: crudActions.handleDeleteProblem
  }
}
