import { useProblemLifecycleActions } from '@/composables/adminProblems/useProblemLifecycleActions'
import { useProblemMutationActions } from '@/composables/adminProblems/useProblemMutationActions'
import { useProblemSampleActions } from '@/composables/adminProblems/useProblemSampleActions'
import { useProblemTestcaseZipActions } from '@/composables/adminProblems/useProblemTestcaseZipActions'

export function useProblemAdminActions({
  authState,
  formatCount,
  busySection,
  feedback,
  newProblemTitle,
  draft,
  problemDetailResource,
  problemCatalogResource,
  loadProblems,
  loadSelectedProblem,
  onCreatedProblem
}){
  const lifecycleActions = useProblemLifecycleActions({
    authState,
    busySection,
    formatCount,
    newProblemTitle,
    feedback,
    selectedProblemDetail: problemDetailResource.selectedProblemDetail,
    loadProblems,
    onCreatedProblem
  })
  const mutationActions = useProblemMutationActions({
    authState,
    busySection,
    formatCount,
    draft,
    feedback,
    problemDetailResource,
    problemCatalogResource
  })
  const sampleActions = useProblemSampleActions({
    authState,
    busySection,
    formatCount,
    draft,
    feedback,
    problemDetailResource
  })
  const testcaseZipActions = useProblemTestcaseZipActions({
    authState,
    busySection,
    formatCount,
    draft,
    feedback,
    problemDetailResource,
    loadSelectedProblem,
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
    canCreateProblem: lifecycleActions.canCreateProblem,
    isCreatingProblem: lifecycleActions.isCreatingProblem,
    isSavingTitle: mutationActions.isSavingTitle,
    isSavingLimits: mutationActions.isSavingLimits,
    isSavingStatement: mutationActions.isSavingStatement,
    isCreatingSample: sampleActions.isCreatingSample,
    isDeletingLastSample: sampleActions.isDeletingLastSample,
    isUploadingTestcaseZip: testcaseZipActions.isUploadingTestcaseZip,
    isRejudgingProblem: lifecycleActions.isRejudgingProblem,
    isDeletingProblem: lifecycleActions.isDeletingProblem,
    canCreateSample: sampleActions.canCreateSample,
    canUploadTestcaseZip: testcaseZipActions.canUploadTestcaseZip,
    canDeleteLastSample: sampleActions.canDeleteLastSample,
    canDeleteSelectedProblem: feedback.canDeleteSelectedProblem,
    canRejudgeSelectedProblem: feedback.canRejudgeSelectedProblem,
    clearActionError: feedback.clearActionError,
    setActionFeedback: feedback.setActionFeedback,
    resetActionState: feedback.resetActionState,
    handleCreateProblem: lifecycleActions.handleCreateProblem,
    handleSaveTitle: mutationActions.handleSaveTitle,
    handleSaveLimits: mutationActions.handleSaveLimits,
    handleSaveStatement: mutationActions.handleSaveStatement,
    handleCreateSample: sampleActions.handleCreateSample,
    handleSaveSample: sampleActions.handleSaveSample,
    handleDeleteLastSample: sampleActions.handleDeleteLastSample,
    handleUploadTestcaseZip: testcaseZipActions.handleUploadTestcaseZip,
    openDeleteDialog: feedback.openDeleteDialog,
    openRejudgeDialog: feedback.openRejudgeDialog,
    closeDeleteDialog: feedback.closeDeleteDialog,
    closeRejudgeDialog: feedback.closeRejudgeDialog,
    handleRejudgeProblem: lifecycleActions.handleRejudgeProblem,
    handleDeleteProblem: lifecycleActions.handleDeleteProblem
  }
}
