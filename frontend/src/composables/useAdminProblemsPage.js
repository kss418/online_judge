import { computed, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'

import { useProblemActionFeedback } from '@/composables/adminProblems/useProblemActionFeedback'
import { useProblemAdminActions } from '@/composables/adminProblems/useProblemAdminActions'
import { useProblemDetailResource } from '@/composables/adminProblems/useProblemDetailResource'
import { useProblemEditorDraft } from '@/composables/adminProblems/useProblemEditorDraft'
import { formatProblemLimit } from '@/composables/adminProblems/problemHelpers'
import { useAdminProblemCatalogQuery } from '@/composables/adminShared/useAdminProblemCatalogQuery'
import { useAdminProblemCatalogResource } from '@/composables/adminShared/useAdminProblemCatalogResource'
import { useAdminProblemWorkspaceBase } from '@/composables/adminShared/useAdminProblemWorkspaceBase'
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
  const selectedProblemId = ref(0)

  const problemQuery = useAdminProblemCatalogQuery({
    route,
    router,
    formatCount,
    selectedProblemId,
    reloadProblems: async (preferredProblemId) => {
      await loadProblems({ preferredProblemId })
    },
    showErrorNotice,
    includeSelectedProblemIdInQuery: true
  })
  const problemListResource = useAdminProblemCatalogResource({
    authState,
    routeQueryState: problemQuery.routeState
  })
  const problemDetailResource = useProblemDetailResource({
    authState,
    selectedProblemId
  })
  const problemActionFeedback = useProblemActionFeedback({
    selectedProblemDetail: problemDetailResource.selectedProblemDetail,
    busySection
  })
  const editorDraft = useProblemEditorDraft({
    authState,
    busySection,
    selectedProblemDetail: problemDetailResource.selectedProblemDetail,
    setActionFeedback: problemActionFeedback.setActionFeedback
  })
  const problemActions = useProblemAdminActions({
    authState,
    formatCount,
    busySection,
    feedback: problemActionFeedback,
    newProblemTitle,
    selectedProblemDetail: problemDetailResource.selectedProblemDetail,
    updateSelectedProblemDetail: problemDetailResource.setSelectedProblemDetail,
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
    applySelectedProblemVersion,
    mergeProblemSummary: problemListResource.mergeProblemSummary,
    loadProblems,
    loadSelectedProblem,
    onCreatedProblem: handleCreatedProblem
  })

  watch(problemActionFeedback.actionMessage, (message) => {
    if (message) {
      showSuccessNotice(message)
    }
  })

  watch(problemActionFeedback.actionErrorMessage, (message) => {
    if (message) {
      showErrorNotice(message, { duration: 5000 })
    }
  })

  function clearSelectedProblemState(){
    selectedProblemId.value = 0
    problemDetailResource.resetSelectedProblemResource()
    editorDraft.resetEditorDrafts()
  }

  function applySelectedProblemVersion(problemId, version){
    const normalizedVersion = Number(version)
    if (!Number.isInteger(normalizedVersion) || normalizedVersion <= 0) {
      return
    }

    problemDetailResource.applySelectedProblemVersion(problemId, normalizedVersion)
    problemListResource.mergeProblemSummary(problemId, {
      version: normalizedVersion
    })
  }

  async function loadSelectedProblem(problemId){
    const normalizedProblemId = Number(problemId)
    if (!Number.isInteger(normalizedProblemId) || normalizedProblemId <= 0) {
      return
    }

    selectedProblemId.value = normalizedProblemId
    problemActionFeedback.clearActionError()
    editorDraft.resetEditorDrafts()

    const result = await problemDetailResource.loadProblemDetail(normalizedProblemId)

    if (result.status !== 'success') {
      return result
    }

    problemListResource.mergeProblemSummary(normalizedProblemId, {
      title: result.data.title,
      version: result.data.version
    })
    editorDraft.assignEditorDrafts(result.data)
    return result
  }

  async function selectProblem(problemId, options = {}){
    const normalizedProblemId = Number(problemId)
    if (!Number.isInteger(normalizedProblemId) || normalizedProblemId <= 0) {
      return
    }

    if (
      !options.force &&
      selectedProblemId.value === normalizedProblemId &&
      problemDetailResource.selectedProblemDetail.value
    ) {
      return
    }

    return loadSelectedProblem(normalizedProblemId)
  }

  async function loadProblems(options = {}){
    if (!authState.token || !canManageProblems.value) {
      return {
        status: 'blocked'
      }
    }

    const preferredProblemId = Number(options.preferredProblemId ?? selectedProblemId.value)
    problemActionFeedback.clearActionError()

    const result = await problemListResource.loadProblems()

    if (result.status === 'error') {
      clearSelectedProblemState()
      return result
    }

    if (result.status !== 'success') {
      return result
    }

    if (!problemListResource.problems.value.length) {
      clearSelectedProblemState()
      return result
    }

    const nextProblemId = problemListResource.problems.value.some((problem) => problem.problem_id === preferredProblemId)
      ? preferredProblemId
      : problemListResource.problems.value[0].problem_id

    await selectProblem(nextProblemId, {
      force: nextProblemId !== selectedProblemId.value || !problemDetailResource.selectedProblemDetail.value
    })

    return result
  }

  async function handleCreatedProblem(problemId){
    problemQuery.searchMode.value = 'title'
    problemQuery.titleSearchInput.value = ''
    problemQuery.problemIdSearchInput.value = ''

    await router.replace({
      query: {
        problemId: String(problemId ?? '')
      }
    })
    await loadProblems({
      preferredProblemId: Number(problemId ?? 0)
    })
  }

  function resetPageState(){
    problemQuery.searchMode.value = 'title'
    problemQuery.titleSearchInput.value = ''
    problemQuery.problemIdSearchInput.value = ''
    newProblemTitle.value = ''
    busySection.value = ''
    problemListResource.resetProblems()
    clearSelectedProblemState()
    problemActionFeedback.resetActionState()
  }

  const problemWorkspace = useAdminProblemWorkspaceBase({
    authState,
    initializeAuth,
    isAuthenticated,
    hasAccess: canManageProblems,
    loggedOutMessage: '문제 관리 페이지는 로그인한 관리자만 사용할 수 있습니다.',
    deniedMessage: '이 페이지는 관리자만 접근할 수 있습니다.',
    routeSearchSources: [
      problemQuery.routeSearchMode,
      problemQuery.routeTitleSearch,
      problemQuery.routeProblemIdSearch
    ],
    syncSearchControlsFromRoute: problemQuery.syncSearchControlsFromRoute,
    loadProblems,
    getInitialPreferredProblemId(){
      return problemQuery.preferredProblemIdFromRoute.value || selectedProblemId.value
    },
    getRefreshPreferredProblemId(){
      return problemQuery.preferredProblemIdForReload.value
    },
    resetPageState,
    preferredProblemIdSource: problemQuery.preferredProblemIdFromRoute
  })
  const pageAccess = problemWorkspace.pageAccess
  const refreshProblems = problemWorkspace.refreshWorkspace
  const isLoadingProblems = computed(() =>
    pageAccess.accessState.value === 'initializing' || problemListResource.isLoadingProblems.value
  )
  const toolbarStatusLabel = computed(() =>
    isLoadingProblems.value ? 'Loading' : `${problemListResource.problemCount.value} Problems`
  )
  const toolbarStatusTone = computed(() =>
    problemListResource.listErrorMessage.value ? 'danger' : 'success'
  )

  return {
    authState,
    accessState: pageAccess.accessState,
    accessMessage: pageAccess.accessMessage,
    isAuthenticated,
    canManageProblems,
    isLoadingProblems,
    isLoadingDetail: problemDetailResource.isLoadingDetail,
    listErrorMessage: problemListResource.listErrorMessage,
    detailErrorMessage: problemDetailResource.detailErrorMessage,
    searchMode: problemQuery.searchMode,
    titleSearchInput: problemQuery.titleSearchInput,
    problemIdSearchInput: problemQuery.problemIdSearchInput,
    newProblemTitle,
    problems: problemListResource.problems,
    selectedProblemId,
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
    rejudgeDialogOpen: problemActionFeedback.rejudgeDialogOpen,
    rejudgeConfirmProblemId: problemActionFeedback.rejudgeConfirmProblemId,
    rejudgeConfirmTitle: problemActionFeedback.rejudgeConfirmTitle,
    deleteDialogOpen: problemActionFeedback.deleteDialogOpen,
    deleteConfirmProblemId: problemActionFeedback.deleteConfirmProblemId,
    deleteConfirmTitle: problemActionFeedback.deleteConfirmTitle,
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
    canDeleteSelectedProblem: problemActionFeedback.canDeleteSelectedProblem,
    canRejudgeSelectedProblem: problemActionFeedback.canRejudgeSelectedProblem,
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
    selectProblem,
    handleCreateProblem: problemActions.handleCreateProblem,
    handleSaveTitle: problemActions.handleSaveTitle,
    handleSaveLimits: problemActions.handleSaveLimits,
    handleSaveStatement: problemActions.handleSaveStatement,
    handleCreateSample: problemActions.handleCreateSample,
    handleSaveSample: problemActions.handleSaveSample,
    handleDeleteLastSample: problemActions.handleDeleteLastSample,
    handleTestcaseZipFileChange: editorDraft.handleTestcaseZipFileChange,
    handleUploadTestcaseZip: problemActions.handleUploadTestcaseZip,
    openDeleteDialog: problemActionFeedback.openDeleteDialog,
    openRejudgeDialog: problemActionFeedback.openRejudgeDialog,
    closeDeleteDialog: problemActionFeedback.closeDeleteDialog,
    closeRejudgeDialog: problemActionFeedback.closeRejudgeDialog,
    handleRejudgeProblem: problemActions.handleRejudgeProblem,
    handleDeleteProblem: problemActions.handleDeleteProblem
  }
}
