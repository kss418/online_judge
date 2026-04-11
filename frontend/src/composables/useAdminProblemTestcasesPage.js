import { computed, nextTick, onMounted, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'

import { useProblemSelectionQuery } from '@/composables/adminProblemTestcases/useProblemSelectionQuery'
import { useProblemTestcaseListResource } from '@/composables/adminProblemTestcases/useProblemTestcaseListResource'
import {
  formatCount as formatCountValue,
  formatProblemLimit as formatProblemLimitValue
} from '@/composables/adminProblemTestcases/testcaseHelpers'
import { useTestcaseEditorDraft } from '@/composables/adminProblemTestcases/useTestcaseEditorDraft'
import { useTestcaseReorder } from '@/composables/adminProblemTestcases/useTestcaseReorder'
import { useTestcaseUploadActions } from '@/composables/adminProblemTestcases/useTestcaseUploadActions'
import { authStore } from '@/stores/auth/authStore'
import { noticeStore } from '@/stores/notice/noticeStore'

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
  const countFormatter = new Intl.NumberFormat()
  const busySection = ref('')
  const testcaseSummaryElementMap = new Map()

  let listResource

  const formatCount = (value) => formatCountValue(countFormatter, value)
  const formatProblemLimit = (value, unit) => formatProblemLimitValue(countFormatter, value, unit)

  const draft = useTestcaseEditorDraft({
    authState,
    busySection,
    testcaseItems: computed(() => listResource?.testcaseItems.value ?? []),
    selectedTestcase: computed(() => listResource?.selectedTestcase.value ?? null),
    showErrorNotice
  })

  const query = useProblemSelectionQuery({
    route,
    router,
    formatCount,
    getSelectedProblemId: () => query.selectedProblemId.value,
    reloadProblems: async (preferredProblemId) => {
      if (!listResource) {
        return
      }

      await listResource.loadProblems({ preferredProblemId })
    },
    showErrorNotice
  })

  listResource = useProblemTestcaseListResource({
    authState,
    selectedProblemId: query.selectedProblemId,
    routeQueryState: query.routeState,
    replaceProblemRoute: query.replaceProblemRoute,
    syncSelectedTestcase: draft.syncSelectedTestcase,
    resetSelectedTestcaseState: draft.resetSelectedTestcaseState
  })

  const uploadActions = useTestcaseUploadActions({
    authState,
    busySection,
    formatCount,
    showErrorNotice,
    showSuccessNotice,
    selectedProblemId: query.selectedProblemId,
    testcaseItems: listResource.testcaseItems,
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
    loadProblems: listResource.loadProblems,
    loadSelectedProblemData: listResource.loadSelectedProblemData,
    loadTestcases: listResource.loadTestcases,
    syncSelectedTestcase: draft.syncSelectedTestcase,
    resetSelectedTestcaseState: draft.resetSelectedTestcaseState
  })

  const reorderActions = useTestcaseReorder({
    authState,
    busySection,
    testcaseItems: listResource.testcaseItems,
    selectedProblemId: query.selectedProblemId,
    selectedTestcaseSummary: draft.selectedTestcaseSummary,
    applyProblemVersion: listResource.applyProblemVersion,
    showErrorNotice,
    showSuccessNotice,
    syncSelectedTestcaseById(preferredTestcaseId, fallbackOrder){
      draft.syncSelectedTestcaseById(preferredTestcaseId, fallbackOrder)
      void scrollSelectedTestcaseIntoView()
    }
  })

  const toolbarStatusLabel = computed(() => {
    if (
      listResource.isLoadingProblems.value ||
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

  watch(query.selectedProblemId, () => {
    resetSelectedProblemState()

    if (!authState.initialized || !canManageProblems.value) {
      return
    }

    if (query.selectedProblemId.value > 0) {
      void listResource.loadSelectedProblemData()
      return
    }

    listResource.isLoadingProblem.value = false
    listResource.isLoadingTestcases.value = false
  })

  watch(listResource.selectedTestcase, (testcase) => {
    draft.selectedTestcaseInputDraft.value = testcase?.testcase_input ?? ''
    draft.selectedTestcaseOutputDraft.value = testcase?.testcase_output ?? ''
  }, {
    immediate: true
  })

  watch(draft.selectedTestcaseSummary, (testcaseSummary) => {
    if (!authState.initialized || !canManageProblems.value) {
      return
    }

    if (!authState.token || query.selectedProblemId.value <= 0 || !testcaseSummary) {
      listResource.clearSelectedTestcaseDetail()
      return
    }

    void listResource.loadSelectedTestcaseDetail(testcaseSummary.testcase_order)
  })

  watch(
    () => [query.routeSearchMode.value, query.routeTitleSearch.value, query.routeProblemIdSearch.value],
    () => {
      if (!authState.initialized || !isAuthenticated.value || !canManageProblems.value) {
        return
      }

      query.syncSearchControlsFromRoute()
      void listResource.loadProblems({
        preferredProblemId:
          query.routeSearchMode.value === 'problem-id'
            ? query.routeProblemIdSearch.value || query.selectedProblemId.value
            : query.selectedProblemId.value
      })
    }
  )

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

  async function refreshPage(){
    if (busySection.value) {
      return
    }

    await listResource.loadProblems({
      preferredProblemId:
        query.routeSearchMode.value === 'problem-id'
          ? query.routeProblemIdSearch.value || query.selectedProblemId.value
          : query.selectedProblemId.value
    })
    await listResource.loadSelectedProblemData()
  }

  onMounted(async () => {
    await initializeAuth()

    if (!canManageProblems.value) {
      listResource.isLoadingProblems.value = false
      listResource.isLoadingProblem.value = false
      listResource.isLoadingTestcases.value = false
      return
    }

    query.syncSearchControlsFromRoute()
    await listResource.loadProblems({
      preferredProblemId:
        query.routeSearchMode.value === 'problem-id'
          ? query.routeProblemIdSearch.value || query.selectedProblemId.value
          : query.selectedProblemId.value
    })
    await listResource.loadSelectedProblemData()
  })

  return {
    authState,
    isAuthenticated,
    canManageProblems,
    selectedProblemId: query.selectedProblemId,
    isLoadingProblems: listResource.isLoadingProblems,
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
