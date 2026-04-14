import { computed, onMounted, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'

import { useProblemAdminActions } from '@/composables/adminProblems/useProblemAdminActions'
import { useProblemDetailResource } from '@/composables/adminProblems/useProblemDetailResource'
import { useProblemEditorDraft } from '@/composables/adminProblems/useProblemEditorDraft'
import { formatProblemLimit } from '@/composables/adminProblems/problemHelpers'
import { useProblemListResource } from '@/composables/adminProblems/useProblemListResource'
import { useProblemSearchQuery } from '@/composables/adminProblems/useProblemSearchQuery'
import { authStore } from '@/stores/auth/authStore'
import { noticeStore } from '@/stores/notice/noticeStore'
import { formatCount } from '@/utils/numberFormat'

export function useAdminProblemsPage(){
  const {
    state: authState,
    isAuthenticated,
    initializeAuth,
    canManageProblems
  } = authStore
  const {
    showErrorNotice,
    showSuccessNotice
  } = noticeStore
  const route = useRoute()
  const router = useRouter()

  const busySection = ref('')
  const newProblemTitle = ref('')

  let problemListResource
  let problemDetailResource

  const editorDraft = useProblemEditorDraft({
    authState,
    busySection,
    getSelectedProblemDetail: () => problemDetailResource?.selectedProblemDetail.value ?? null,
    setActionFeedback: ({ message, error }) => {
      if (!problemActions) {
        return
      }

      problemActions.setActionFeedback({ message, error })
    }
  })

  const problemQuery = useProblemSearchQuery({
    route,
    router,
    formatCount,
    selectedProblemId: computed(() => problemDetailResource?.selectedProblemId.value ?? 0),
    reloadProblems: async (preferredProblemId) => {
      if (!problemListResource) {
        return
      }

      await problemListResource.loadProblems({ preferredProblemId })
    },
    showErrorNotice
  })

  problemDetailResource = useProblemDetailResource({
    authState,
    mergeProblemSummary(problemId, patch){
      problemListResource?.mergeProblemSummary(problemId, patch)
    },
    resetEditorDrafts: editorDraft.resetEditorDrafts,
    assignEditorDrafts: editorDraft.assignEditorDrafts,
    clearActionError(){
      problemActions?.clearActionError()
    }
  })

  problemListResource = useProblemListResource({
    authState,
    canManageProblems,
    routeQueryState: problemQuery.routeState,
    selectedProblemId: problemDetailResource.selectedProblemId,
    selectedProblemDetail: problemDetailResource.selectedProblemDetail,
    detailErrorMessage: problemDetailResource.detailErrorMessage,
    resetEditorDrafts: editorDraft.resetEditorDrafts,
    clearActionError(){
      problemActions?.clearActionError()
    },
    invalidateSelectedProblemResource: problemDetailResource.invalidateSelectedProblemResource,
    selectProblem: problemDetailResource.selectProblem
  })

  async function handleCreatedProblem(problemId){
    problemQuery.searchMode.value = 'title'
    problemQuery.titleSearchInput.value = ''
    problemQuery.problemIdSearchInput.value = ''

    await router.replace({
      query: {
        problemId: String(problemId ?? '')
      }
    })
    await problemListResource.loadProblems({
      preferredProblemId: Number(problemId ?? 0)
    })
  }

  let problemActions = useProblemAdminActions({
    authState,
    formatCount,
    busySection,
    newProblemTitle,
    selectedProblemDetail: problemDetailResource.selectedProblemDetail,
    testcaseZipFile: editorDraft.testcaseZipFile,
    titleDraft: editorDraft.titleDraft,
    timeLimitDraft: editorDraft.timeLimitDraft,
    memoryLimitDraft: editorDraft.memoryLimitDraft,
    descriptionDraft: editorDraft.descriptionDraft,
    inputFormatDraft: editorDraft.inputFormatDraft,
    outputFormatDraft: editorDraft.outputFormatDraft,
    noteDraft: editorDraft.noteDraft,
    canSaveTitle: editorDraft.canSaveTitle,
    canSaveLimits: editorDraft.canSaveLimits,
    canSaveStatement: editorDraft.canSaveStatement,
    canSaveSample: editorDraft.canSaveSample,
    getSampleDraft: editorDraft.getSampleDraft,
    syncSampleDrafts: editorDraft.syncSampleDrafts,
    setSelectedProblemSamples: problemDetailResource.setSelectedProblemSamples,
    applySelectedProblemVersion: problemDetailResource.applySelectedProblemVersion,
    mergeProblemSummary: problemListResource.mergeProblemSummary,
    loadProblems: problemListResource.loadProblems,
    loadSelectedProblem: problemDetailResource.loadSelectedProblem,
    onCreatedProblem: handleCreatedProblem
  })

  const toolbarStatusLabel = computed(() =>
    problemListResource.isLoadingProblems.value ? 'Loading' : `${problemListResource.problemCount.value} Problems`
  )
  const toolbarStatusTone = computed(() =>
    problemListResource.listErrorMessage.value ? 'danger' : 'success'
  )

  watch(problemActions.actionMessage, (message) => {
    if (message) {
      showSuccessNotice(message)
    }
  })

  watch(problemActions.actionErrorMessage, (message) => {
    if (message) {
      showErrorNotice(message, { duration: 5000 })
    }
  })

  watch(
    () => [authState.initialized, authState.token, canManageProblems.value],
    () => {
      if (!authState.initialized) {
        problemListResource.isLoadingProblems.value = true
        return
      }

      if (!isAuthenticated.value || !canManageProblems.value) {
        resetPageState()
        problemListResource.isLoadingProblems.value = false
        return
      }

      problemQuery.syncSearchControlsFromRoute()
      void problemListResource.loadProblems({
        preferredProblemId:
          problemQuery.preferredProblemIdFromRoute.value || problemDetailResource.selectedProblemId.value
      })
    },
    { immediate: true }
  )

  watch(problemQuery.preferredProblemIdFromRoute, (problemId) => {
    if (!authState.initialized || !isAuthenticated.value || !canManageProblems.value || !problemId) {
      return
    }

    void problemListResource.loadProblems({ preferredProblemId: problemId })
  })

  watch(
    () => [
      problemQuery.routeSearchMode.value,
      problemQuery.routeTitleSearch.value,
      problemQuery.routeProblemIdSearch.value
    ],
    () => {
      if (!authState.initialized || !isAuthenticated.value || !canManageProblems.value) {
        return
      }

      problemQuery.syncSearchControlsFromRoute()
      void problemListResource.loadProblems({
        preferredProblemId: problemQuery.preferredProblemIdForReload.value
      })
    }
  )

  function resetPageState(){
    problemQuery.searchMode.value = 'title'
    problemQuery.titleSearchInput.value = ''
    problemQuery.problemIdSearchInput.value = ''
    newProblemTitle.value = ''
    problemListResource.problems.value = []
    problemDetailResource.selectedProblemId.value = 0
    problemDetailResource.selectedProblemDetail.value = null
    problemListResource.listErrorMessage.value = ''
    problemDetailResource.detailErrorMessage.value = ''
    busySection.value = ''
    problemActions.resetActionState()
    editorDraft.resetEditorDrafts()
  }

  async function refreshProblems(){
    await problemListResource.loadProblems({
      preferredProblemId: problemDetailResource.selectedProblemId.value
    })
  }

  onMounted(() => {
    void initializeAuth()
  })

  return {
    authState,
    isAuthenticated,
    canManageProblems,
    isLoadingProblems: problemListResource.isLoadingProblems,
    isLoadingDetail: problemDetailResource.isLoadingDetail,
    listErrorMessage: problemListResource.listErrorMessage,
    detailErrorMessage: problemDetailResource.detailErrorMessage,
    searchMode: problemQuery.searchMode,
    titleSearchInput: problemQuery.titleSearchInput,
    problemIdSearchInput: problemQuery.problemIdSearchInput,
    newProblemTitle,
    problems: problemListResource.problems,
    selectedProblemId: problemDetailResource.selectedProblemId,
    selectedProblemDetail: problemDetailResource.selectedProblemDetail,
    titleDraft: editorDraft.titleDraft,
    timeLimitDraft: editorDraft.timeLimitDraft,
    memoryLimitDraft: editorDraft.memoryLimitDraft,
    descriptionDraft: editorDraft.descriptionDraft,
    inputFormatDraft: editorDraft.inputFormatDraft,
    outputFormatDraft: editorDraft.outputFormatDraft,
    noteDraft: editorDraft.noteDraft,
    sampleDrafts: editorDraft.sampleDrafts,
    testcaseZipInputKey: editorDraft.testcaseZipInputKey,
    selectedTestcaseZipName: editorDraft.selectedTestcaseZipName,
    busySection,
    rejudgeDialogOpen: problemActions.rejudgeDialogOpen,
    rejudgeConfirmProblemId: problemActions.rejudgeConfirmProblemId,
    rejudgeConfirmTitle: problemActions.rejudgeConfirmTitle,
    deleteDialogOpen: problemActions.deleteDialogOpen,
    deleteConfirmProblemId: problemActions.deleteConfirmProblemId,
    deleteConfirmTitle: problemActions.deleteConfirmTitle,
    problemCount: problemListResource.problemCount,
    canCreateProblem: problemActions.canCreateProblem,
    isCreatingProblem: problemActions.isCreatingProblem,
    isSavingTitle: problemActions.isSavingTitle,
    isSavingLimits: problemActions.isSavingLimits,
    isSavingStatement: problemActions.isSavingStatement,
    isCreatingSample: problemActions.isCreatingSample,
    isDeletingLastSample: problemActions.isDeletingLastSample,
    isUploadingTestcaseZip: problemActions.isUploadingTestcaseZip,
    isRejudgingProblem: problemActions.isRejudgingProblem,
    isDeletingProblem: problemActions.isDeletingProblem,
    canCreateSample: problemActions.canCreateSample,
    canUploadTestcaseZip: problemActions.canUploadTestcaseZip,
    canDeleteLastSample: problemActions.canDeleteLastSample,
    canSaveTitle: editorDraft.canSaveTitle,
    canSaveLimits: editorDraft.canSaveLimits,
    canSaveStatement: editorDraft.canSaveStatement,
    canDeleteSelectedProblem: problemActions.canDeleteSelectedProblem,
    canRejudgeSelectedProblem: problemActions.canRejudgeSelectedProblem,
    hasAppliedSearch: problemQuery.hasAppliedSearch,
    problemListCaption: problemQuery.problemListCaption,
    emptyProblemListMessage: problemQuery.emptyProblemListMessage,
    toolbarStatusLabel,
    toolbarStatusTone,
    formatCount,
    formatProblemLimit,
    isSavingSample: editorDraft.isSavingSample,
    canSaveSample: editorDraft.canSaveSample,
    isLastSample: editorDraft.isLastSample,
    setSearchMode: problemQuery.setSearchMode,
    handleProblemIdSearchInput: problemQuery.handleProblemIdSearchInput,
    submitSearch: problemQuery.submitSearch,
    resetSearch: problemQuery.resetSearch,
    refreshProblems,
    selectProblem: problemDetailResource.selectProblem,
    handleCreateProblem: problemActions.handleCreateProblem,
    handleSaveTitle: problemActions.handleSaveTitle,
    handleSaveLimits: problemActions.handleSaveLimits,
    handleSaveStatement: problemActions.handleSaveStatement,
    handleCreateSample: problemActions.handleCreateSample,
    handleSaveSample: problemActions.handleSaveSample,
    handleDeleteLastSample: problemActions.handleDeleteLastSample,
    handleTestcaseZipFileChange: editorDraft.handleTestcaseZipFileChange,
    handleUploadTestcaseZip: problemActions.handleUploadTestcaseZip,
    openDeleteDialog: problemActions.openDeleteDialog,
    openRejudgeDialog: problemActions.openRejudgeDialog,
    closeDeleteDialog: problemActions.closeDeleteDialog,
    closeRejudgeDialog: problemActions.closeRejudgeDialog,
    handleRejudgeProblem: problemActions.handleRejudgeProblem,
    handleDeleteProblem: problemActions.handleDeleteProblem
  }
}
