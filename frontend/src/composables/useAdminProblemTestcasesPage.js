import { computed, nextTick, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'

import { useProblemSelectionQuery } from '@/composables/adminProblemTestcases/useProblemSelectionQuery'
import { useProblemTestcaseListResource } from '@/composables/adminProblemTestcases/useProblemTestcaseListResource'
import { useAdminProblemWorkspaceBase } from '@/composables/adminShared/useAdminProblemWorkspaceBase'
import { formatProblemLimit } from '@/composables/adminProblemTestcases/testcaseHelpers'
import { useTestcaseEditorDraft } from '@/composables/adminProblemTestcases/useTestcaseEditorDraft'
import { useTestcaseReorder } from '@/composables/adminProblemTestcases/useTestcaseReorder'
import { useTestcaseUploadActions } from '@/composables/adminProblemTestcases/useTestcaseUploadActions'
import { authStore } from '@/stores/auth/authStore'
import { noticeStore } from '@/stores/notice/noticeStore'
import { formatCount } from '@/utils/numberFormat'

export function useAdminProblemTestcasesPage(){
  const route = useRoute()
  const router = useRouter()
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
  const busySection = ref('')
  const testcaseSummaryElementMap = new Map()

  const query = useProblemSelectionQuery({
    route,
    router,
    formatCount,
    reloadProblems: async (preferredProblemId) => {
      await loadProblems({ preferredProblemId })
    },
    showErrorNotice
  })
  const listResource = useProblemTestcaseListResource({
    authState,
    selectedProblemId: query.selectedProblemId,
    routeQueryState: query.routeState
  })
  const draft = useTestcaseEditorDraft({
    authState,
    busySection,
    testcaseItems: listResource.testcaseItems,
    selectedTestcase: listResource.selectedTestcase,
    showErrorNotice
  })
  const uploadActions = useTestcaseUploadActions({
    authState,
    busySection,
    formatCount,
    showErrorNotice,
    showSuccessNotice,
    selectedProblemId: query.selectedProblemId,
    selectedTestcase: listResource.selectedTestcase,
    selectedTestcaseSummary: draft.selectedTestcaseSummary,
    newTestcaseInput: draft.newTestcaseInput,
    newTestcaseOutput: draft.newTestcaseOutput,
    testcaseZipFile: draft.testcaseZipFile,
    testcaseZipInputKey: draft.testcaseZipInputKey,
    selectedTestcaseInputDraft: draft.selectedTestcaseInputDraft,
    selectedTestcaseOutputDraft: draft.selectedTestcaseOutputDraft,
    canSaveSelectedTestcase: draft.canSaveSelectedTestcase,
    applyProblemVersion: listResource.applyProblemVersion,
    reloadProblems: loadProblems,
    reloadSelectedProblemData: loadSelectedProblemData,
    reloadTestcases: loadTestcases,
    updateTestcaseItems: listResource.setTestcaseItems,
    setSelectedTestcaseDetail: listResource.setSelectedTestcase,
    syncSelectedTestcase
  })
  const reorderActions = useTestcaseReorder({
    authState,
    busySection,
    testcaseItems: listResource.testcaseItems,
    selectedProblemId: query.selectedProblemId,
    selectedTestcaseSummary: draft.selectedTestcaseSummary,
    applyProblemVersion: listResource.applyProblemVersion,
    updateTestcaseItems: listResource.setTestcaseItems,
    showErrorNotice,
    showSuccessNotice,
    syncSelectedTestcaseById(preferredTestcaseId, fallbackOrder){
      draft.syncSelectedTestcaseById(preferredTestcaseId, fallbackOrder)
      void scrollSelectedTestcaseIntoView()
    }
  })

  const problemWorkspace = useAdminProblemWorkspaceBase({
    authState,
    initializeAuth,
    isAuthenticated,
    hasAccess: canManageProblems,
    loggedOutMessage: '테스트케이스 관리 페이지는 로그인한 관리자만 사용할 수 있습니다.',
    deniedMessage: '이 페이지는 관리자만 접근할 수 있습니다.',
    routeSearchSources: [
      query.routeSearchMode,
      query.routeTitleSearch,
      query.routeProblemIdSearch
    ],
    syncSearchControlsFromRoute: query.syncSearchControlsFromRoute,
    loadProblems,
    getInitialPreferredProblemId(){
      return query.preferredProblemIdForReload.value
    },
    getRefreshPreferredProblemId(){
      return query.preferredProblemIdForReload.value
    },
    resetPageState,
    afterInitialProblemLoad: loadSelectedProblemData,
    afterRefreshProblemLoad: loadSelectedProblemData,
    selectedProblemIdSource: query.selectedProblemId,
    resetSelectedProblemState,
    reloadSelectedProblemData: loadSelectedProblemData,
    isRefreshBlocked(){
      return Boolean(busySection.value)
    }
  })
  const pageAccess = problemWorkspace.pageAccess
  const refreshPage = problemWorkspace.refreshWorkspace
  const isLoadingProblems = computed(() =>
    pageAccess.accessState.value === 'initializing' || listResource.isLoadingProblems.value
  )
  const toolbarStatusLabel = computed(() => {
    if (
      isLoadingProblems.value ||
      listResource.isLoadingProblem.value ||
      listResource.isLoadingTestcases.value
    ) {
      return 'Loading'
    }

    if (query.selectedProblemId.value > 0) {
      return `${formatCount(listResource.testcaseCount.value)} Testcases`
    }

    return `${formatCount(listResource.problemCount.value)} Problems`
  })
  const toolbarStatusTone = computed(() => {
    if (
      listResource.listErrorMessage.value ||
      listResource.problemErrorMessage.value ||
      listResource.testcaseErrorMessage.value
    ) {
      return 'danger'
    }

    return 'success'
  })

  watch(listResource.selectedTestcase, (testcase) => {
    draft.selectedTestcaseInputDraft.value = testcase?.testcase_input ?? ''
    draft.selectedTestcaseOutputDraft.value = testcase?.testcase_output ?? ''
  }, {
    immediate: true
  })

  pageAccess.watchWhenAllowed(draft.selectedTestcaseSummary, (testcaseSummary) => {
    if (!authState.token || query.selectedProblemId.value <= 0 || !testcaseSummary) {
      listResource.clearSelectedTestcaseDetail()
      return
    }

    void listResource.loadSelectedTestcaseDetail(testcaseSummary.testcase_order)
  })

  function describeTestcaseContent(charCount, lineCount){
    const normalizedCharCount = Number(charCount ?? 0)
    const normalizedLineCount = Number(lineCount ?? 0)

    if (normalizedCharCount <= 0) {
      return '빈 값'
    }

    return `${formatCount(normalizedCharCount)}자 · ${formatCount(normalizedLineCount)}줄`
  }

  function isLastTestcase(testcaseOrder){
    if (!listResource.testcaseItems.value.length) {
      return false
    }

    return listResource.testcaseItems.value[listResource.testcaseItems.value.length - 1].testcase_order === testcaseOrder
  }

  function resetSelectedProblemState(){
    listResource.resetSelectedProblemResource()
    draft.resetDraftState()
  }

  function resetSelectedTestcaseState(){
    draft.resetSelectedTestcaseState()
    listResource.clearSelectedTestcaseDetail()
  }

  function syncSelectedTestcase(preferredOrder){
    draft.syncSelectedTestcase(preferredOrder)
  }

  async function loadProblems(options = {}){
    const preferredProblemId = Number(options.preferredProblemId ?? query.selectedProblemId.value)
    const result = await listResource.loadProblems()

    if (result.status !== 'success') {
      return result
    }

    if (!listResource.problems.value.length) {
      if (query.selectedProblemId.value > 0) {
        await query.replaceProblemRoute(0)
      } else {
        resetSelectedProblemState()
      }
      return result
    }

    const nextProblemId = listResource.problems.value.some((problem) => problem.problem_id === preferredProblemId)
      ? preferredProblemId
      : listResource.problems.value[0].problem_id
    if (nextProblemId > 0 && nextProblemId !== query.selectedProblemId.value) {
      await query.replaceProblemRoute(nextProblemId)
    }

    return result
  }

  async function loadTestcases(preferredOrder){
    const result = await listResource.loadTestcases()

    if (result.status !== 'success') {
      resetSelectedTestcaseState()
      return result
    }

    syncSelectedTestcase(preferredOrder)
    return result
  }

  async function loadSelectedProblemData(){
    if (query.selectedProblemId.value <= 0) {
      listResource.resetSelectedProblemResource()
      resetSelectedTestcaseState()
      return {
        status: 'reset'
      }
    }

    const [problemResult, testcaseResult] = await Promise.all([
      listResource.loadProblemDetail(),
      loadTestcases()
    ])

    return problemResult?.status === 'error' ? problemResult : testcaseResult
  }

  function resetPageState(){
    query.searchMode.value = 'title'
    query.titleSearchInput.value = ''
    query.problemIdSearchInput.value = ''
    busySection.value = ''
    testcaseSummaryElementMap.clear()
    listResource.resetProblemListResource()
    listResource.resetSelectedProblemResource()
    draft.resetDraftState()
  }

  function selectTestcase(testcaseOrder){
    draft.selectTestcase(testcaseOrder)
    void scrollSelectedTestcaseIntoView()
  }

  function setTestcaseSummaryElement(testcaseOrder, element){
    if (!element) {
      testcaseSummaryElementMap.delete(testcaseOrder)
      return
    }

    testcaseSummaryElementMap.set(testcaseOrder, element)
  }

  async function scrollSelectedTestcaseIntoView(){
    await nextTick()

    const summaryElement = testcaseSummaryElementMap.get(draft.selectedTestcaseOrder.value)
    if (!summaryElement || typeof summaryElement.scrollIntoView !== 'function') {
      return
    }

    summaryElement.scrollIntoView({
      block: 'nearest',
      inline: 'nearest'
    })
  }

  function handleViewSelectedTestcase(){
    if (draft.handleViewSelectedTestcase()) {
      void scrollSelectedTestcaseIntoView()
    }
  }

  return {
    authState,
    accessState: pageAccess.accessState,
    accessMessage: pageAccess.accessMessage,
    isAuthenticated,
    canManageProblems,
    selectedProblemId: query.selectedProblemId,
    isLoadingProblems,
    isLoadingProblem: listResource.isLoadingProblem,
    isLoadingTestcases: listResource.isLoadingTestcases,
    isLoadingSelectedTestcase: listResource.isLoadingSelectedTestcase,
    listErrorMessage: listResource.listErrorMessage,
    problemErrorMessage: listResource.problemErrorMessage,
    testcaseErrorMessage: listResource.testcaseErrorMessage,
    selectedTestcaseErrorMessage: listResource.selectedTestcaseErrorMessage,
    searchMode: query.searchMode,
    titleSearchInput: query.titleSearchInput,
    problemIdSearchInput: query.problemIdSearchInput,
    problems: listResource.problems,
    problemDetail: listResource.problemDetail,
    testcaseItems: listResource.testcaseItems,
    newTestcaseInput: draft.newTestcaseInput,
    newTestcaseOutput: draft.newTestcaseOutput,
    testcaseZipInputKey: draft.testcaseZipInputKey,
    selectedTestcaseOrder: draft.selectedTestcaseOrder,
    selectedTestcaseInputDraft: draft.selectedTestcaseInputDraft,
    selectedTestcaseOutputDraft: draft.selectedTestcaseOutputDraft,
    viewTestcaseOrderInput: draft.viewTestcaseOrderInput,
    busySection,
    problemCount: listResource.problemCount,
    toolbarStatusLabel,
    toolbarStatusTone,
    hasAppliedSearch: query.hasAppliedSearch,
    problemListCaption: query.problemListCaption,
    emptyProblemListMessage: query.emptyProblemListMessage,
    isCreatingTestcase: uploadActions.isCreatingTestcase,
    isUploadingTestcaseZip: uploadActions.isUploadingTestcaseZip,
    isDeletingSelectedTestcase: uploadActions.isDeletingSelectedTestcase,
    isMovingTestcase: reorderActions.isMovingTestcase,
    isSavingSelectedTestcase: uploadActions.isSavingSelectedTestcase,
    selectedTestcaseZipName: draft.selectedTestcaseZipName,
    selectedTestcase: listResource.selectedTestcase,
    canCreateTestcase: uploadActions.canCreateTestcase,
    canUploadTestcaseZip: uploadActions.canUploadTestcaseZip,
    canDeleteSelectedTestcase: uploadActions.canDeleteSelectedTestcase,
    canMoveTestcases: reorderActions.canMoveTestcases,
    canSaveSelectedTestcase: draft.canSaveSelectedTestcase,
    canViewSpecificTestcase: draft.canViewSpecificTestcase,
    formatCount,
    formatProblemLimit,
    describeTestcaseContent,
    isLastTestcase,
    setSearchMode: query.setSearchMode,
    handleProblemIdSearchInput: query.handleProblemIdSearchInput,
    submitSearch: query.submitSearch,
    resetSearch: query.resetSearch,
    selectProblem: query.selectProblem,
    refreshPage,
    handleTestcaseZipFileChange: draft.handleTestcaseZipFileChange,
    handleUploadTestcaseZip: uploadActions.handleUploadTestcaseZip,
    handleCreateTestcase: uploadActions.handleCreateTestcase,
    selectTestcase,
    handleDeleteSelectedTestcase: uploadActions.handleDeleteSelectedTestcase,
    handleMoveTestcase: reorderActions.handleMoveTestcase,
    handleSaveSelectedTestcase: uploadActions.handleSaveSelectedTestcase,
    handleViewSelectedTestcase,
    setTestcaseSummaryElement
  }
}
