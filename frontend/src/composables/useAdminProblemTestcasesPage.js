import { computed, nextTick, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'

import { useSelectedTestcaseResource } from '@/composables/adminProblemTestcases/useSelectedTestcaseResource'
import { formatProblemLimit } from '@/composables/adminProblemTestcases/testcaseHelpers'
import { useTestcaseEditorDraft } from '@/composables/adminProblemTestcases/useTestcaseEditorDraft'
import { useTestcaseReorder } from '@/composables/adminProblemTestcases/useTestcaseReorder'
import { useTestcaseListResource } from '@/composables/adminProblemTestcases/useTestcaseListResource'
import { useTestcaseUploadActions } from '@/composables/adminProblemTestcases/useTestcaseUploadActions'
import { useAdminProblemCatalogQuery } from '@/composables/adminShared/useAdminProblemCatalogQuery'
import { useAdminProblemCatalogResource } from '@/composables/adminShared/useAdminProblemCatalogResource'
import { useAdminProblemRouteCatalogReload } from '@/composables/adminShared/useAdminProblemRouteCatalogReload'
import { useAdminProblemSelectionReload } from '@/composables/adminShared/useAdminProblemSelectionReload'
import { useProtectedAdminPageAccess } from '@/composables/adminShared/useProtectedAdminPageAccess'
import { useSelectedProblemDetailResource } from '@/composables/adminShared/useSelectedProblemDetailResource'
import { authStore } from '@/stores/auth/authStore'
import { noticeStore } from '@/stores/notice/noticeStore'
import { formatCount } from '@/utils/numberFormat'
import { parsePositiveInteger } from '@/utils/parse'

function sanitizeNumericInput(value){
  return String(value ?? '').replace(/\D+/g, '')
}

function createTestcaseEditorViewModel({
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
      viewTestcaseOrderInput.value = sanitizeNumericInput(value)
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
  const selectedProblemId = computed(() => parsePositiveInteger(route.params.problemId) ?? 0)

  const query = useAdminProblemCatalogQuery({
    route,
    router,
    formatCount,
    selectedProblemId,
    reloadProblems: async (preferredProblemId) => {
      await loadProblems({ preferredProblemId })
    },
    showErrorNotice,
    buildLocation({ query: nextQuery, selectedProblemId: nextSelectedProblemId }){
      const routeProblemId = Number.isInteger(Number(nextSelectedProblemId))
        ? Number(nextSelectedProblemId)
        : selectedProblemId.value

      return {
        name: 'admin-problem-testcases',
        params: routeProblemId > 0
          ? {
            problemId: String(routeProblemId)
          }
          : {},
        query: nextQuery
      }
    }
  })
  const problemCatalogResource = useAdminProblemCatalogResource({
    authState,
    routeQueryState: query.routeState
  })
  const problemDetailResource = useSelectedProblemDetailResource({
    authState,
    selectedProblemId,
    mergeProblemSummary: problemCatalogResource.mergeProblemSummary
  })
  const testcaseListResource = useTestcaseListResource({
    authState,
    selectedProblemId
  })
  const selectedTestcaseResource = useSelectedTestcaseResource({
    authState,
    selectedProblemId
  })

  async function selectProblem(problemId){
    if (problemId === selectedProblemId.value) {
      return
    }

    await query.replaceSelectedProblem(problemId, {
      push: true
    })
  }

  const draft = useTestcaseEditorDraft({
    authState,
    busySection,
    testcaseItems: testcaseListResource.testcaseItems,
    selectedTestcase: selectedTestcaseResource.selectedTestcase,
    showErrorNotice
  })
  const uploadActions = useTestcaseUploadActions({
    authState,
    busySection,
    formatCount,
    showErrorNotice,
    showSuccessNotice,
    selectedProblemId,
    selectedTestcase: selectedTestcaseResource.selectedTestcase,
    selectedTestcaseSummary: draft.selectedTestcaseSummary,
    newTestcaseInput: draft.newTestcaseInput,
    newTestcaseOutput: draft.newTestcaseOutput,
    testcaseZipFile: draft.testcaseZipFile,
    resetTestcaseZipSelection: draft.resetTestcaseZipSelection,
    selectedTestcaseInputDraft: draft.selectedTestcaseInputDraft,
    selectedTestcaseOutputDraft: draft.selectedTestcaseOutputDraft,
    canSaveSelectedTestcase: draft.canSaveSelectedTestcase,
    applyProblemVersion: problemDetailResource.applyProblemVersion,
    reloadProblems: loadProblems,
    reloadSelectedProblemData: loadSelectedProblemData,
    reloadTestcases: loadTestcases,
    updateTestcaseItems: testcaseListResource.setTestcaseItems,
    setSelectedTestcaseDetail: selectedTestcaseResource.setSelectedTestcase,
    syncSelectedTestcase
  })
  const reorderActions = useTestcaseReorder({
    authState,
    busySection,
    testcaseItems: testcaseListResource.testcaseItems,
    selectedProblemId,
    selectedTestcaseSummary: draft.selectedTestcaseSummary,
    applyProblemVersion: problemDetailResource.applyProblemVersion,
    updateTestcaseItems: testcaseListResource.setTestcaseItems,
    showErrorNotice,
    showSuccessNotice,
    syncSelectedTestcaseById(preferredTestcaseId, fallbackOrder){
      draft.syncSelectedTestcaseById(preferredTestcaseId, fallbackOrder)
      void scrollSelectedTestcaseIntoView()
    }
  })

  const pageAccess = useProtectedAdminPageAccess({
    authState,
    initializeAuth,
    isAuthenticated,
    hasAccess: canManageProblems,
    onDenied: resetPageState,
    async onAllowed(){
      await query.syncFromRouteAndReload()
      await loadSelectedProblemData()
    },
    loggedOutMessage: '테스트케이스 관리 페이지는 로그인한 관리자만 사용할 수 있습니다.',
    deniedMessage: '이 페이지는 관리자만 접근할 수 있습니다.'
  })

  useAdminProblemRouteCatalogReload({
    pageAccess,
    query
  })

  useAdminProblemSelectionReload({
    pageAccess,
    selectedProblemId,
    resetSelectedProblemState,
    reloadSelectedProblemData: loadSelectedProblemData
  })

  async function refreshPage(){
    if (busySection.value) {
      return
    }

    await loadProblems({
      preferredProblemId: query.preferredProblemIdForReload.value
    })
    await loadSelectedProblemData()
  }

  const isLoadingProblems = computed(() =>
    pageAccess.accessState.value === 'initializing' || problemCatalogResource.isLoadingProblems.value
  )
  const toolbarStatusLabel = computed(() => {
    if (
      isLoadingProblems.value ||
      problemDetailResource.isLoadingDetail.value ||
      testcaseListResource.isLoadingTestcases.value
    ) {
      return 'Loading'
    }

    if (selectedProblemId.value > 0) {
      return `${formatCount(testcaseListResource.testcaseCount.value)} Testcases`
    }

    return `${formatCount(problemCatalogResource.problemCount.value)} Problems`
  })
  const toolbarStatusTone = computed(() => {
    if (
      problemCatalogResource.listErrorMessage.value ||
      problemDetailResource.detailErrorMessage.value ||
      testcaseListResource.testcaseErrorMessage.value
    ) {
      return 'danger'
    }

    return 'success'
  })

  watch(selectedTestcaseResource.selectedTestcase, (testcase) => {
    draft.selectedTestcaseInputDraft.value = testcase?.testcase_input ?? ''
    draft.selectedTestcaseOutputDraft.value = testcase?.testcase_output ?? ''
  }, {
    immediate: true
  })

  pageAccess.watchWhenAllowed(draft.selectedTestcaseSummary, (testcaseSummary) => {
    if (!authState.token || selectedProblemId.value <= 0 || !testcaseSummary) {
      selectedTestcaseResource.clearSelectedTestcaseDetail()
      return
    }

    void selectedTestcaseResource.loadSelectedTestcaseDetail(testcaseSummary.testcase_order)
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
    if (!testcaseListResource.testcaseItems.value.length) {
      return false
    }

    return testcaseListResource.testcaseItems.value[
      testcaseListResource.testcaseItems.value.length - 1
    ].testcase_order === testcaseOrder
  }

  function resetSelectedProblemState(){
    problemDetailResource.resetSelectedProblemDetail()
    testcaseListResource.resetTestcaseList()
    selectedTestcaseResource.clearSelectedTestcaseDetail()
    draft.resetDraftState()
  }

  function resetSelectedTestcaseState(){
    draft.resetSelectedTestcaseState()
    selectedTestcaseResource.clearSelectedTestcaseDetail()
  }

  function syncSelectedTestcase(preferredOrder){
    draft.syncSelectedTestcase(preferredOrder)
  }

  async function loadProblems(options = {}){
    const preferredProblemId = Number(options.preferredProblemId ?? selectedProblemId.value)
    const result = await problemCatalogResource.loadProblems()

    if (result.status !== 'success') {
      return result
    }

    if (!problemCatalogResource.problems.value.length) {
      if (selectedProblemId.value > 0) {
        await query.replaceSelectedProblem(0)
      } else {
        resetSelectedProblemState()
      }
      return result
    }

    const nextProblemId = problemCatalogResource.problems.value.some((problem) => problem.problem_id === preferredProblemId)
      ? preferredProblemId
      : problemCatalogResource.problems.value[0].problem_id
    if (nextProblemId > 0 && nextProblemId !== selectedProblemId.value) {
      await query.replaceSelectedProblem(nextProblemId)
    }

    return result
  }

  async function loadTestcases(preferredOrder){
    const result = await testcaseListResource.loadTestcases()

    if (result.status !== 'success') {
      resetSelectedTestcaseState()
      return result
    }

    syncSelectedTestcase(preferredOrder)
    return result
  }

  async function loadSelectedProblemData(){
    if (selectedProblemId.value <= 0) {
      problemDetailResource.resetSelectedProblemDetail()
      testcaseListResource.resetTestcaseList()
      resetSelectedTestcaseState()
      return {
        status: 'reset'
      }
    }

    const [problemResult, testcaseResult] = await Promise.all([
      problemDetailResource.loadProblemDetail(),
      loadTestcases()
    ])

    return problemResult?.status === 'error' ? problemResult : testcaseResult
  }

  function resetPageState(){
    query.resetSearchControls()
    busySection.value = ''
    testcaseSummaryElementMap.clear()
    problemCatalogResource.resetProblems()
    problemDetailResource.resetSelectedProblemDetail()
    testcaseListResource.resetTestcaseList()
    selectedTestcaseResource.clearSelectedTestcaseDetail()
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

  const shell = computed(() => ({
    state: pageAccess.accessState.value,
    message: pageAccess.accessMessage.value
  }))
  const toolbar = computed(() => ({
    model: {
      statusLabel: toolbarStatusLabel.value,
      statusTone: toolbarStatusTone.value,
      canManageProblems: canManageProblems.value,
      busySection: busySection.value,
      isLoadingProblems: isLoadingProblems.value,
      isLoadingProblem: problemDetailResource.isLoadingDetail.value,
      isLoadingTestcases: testcaseListResource.isLoadingTestcases.value,
      selectedProblemId: selectedProblemId.value
    },
    actions: {
      refresh: refreshPage
    }
  }))
  const sidebar = computed(() => ({
    model: {
      searchMode: query.searchMode.value,
      titleSearchInput: query.titleSearchInput.value,
      problemIdSearchInput: query.problemIdSearchInput.value,
      titleSearchInputId: 'admin-testcase-problem-search',
      problemIdSearchInputId: 'admin-testcase-problem-id-search',
      isLoadingProblems: isLoadingProblems.value,
      busySection: busySection.value,
      hasAppliedSearch: query.hasAppliedSearch.value,
      problemListCaption: query.problemListCaption.value,
      problemCount: problemCatalogResource.problemCount.value,
      listErrorMessage: problemCatalogResource.listErrorMessage.value,
      emptyProblemListMessage: query.emptyProblemListMessage.value,
      problems: problemCatalogResource.problems.value,
      selectedProblemId: selectedProblemId.value,
      formatCount,
      formatProblemLimit
    },
    actions: {
      updateTitleSearchInput: query.updateTitleSearchInput,
      updateProblemIdSearchInput: query.updateProblemIdSearchInput,
      setSearchMode: query.setSearchMode,
      submitSearch: query.submitSearch,
      resetSearch: query.resetSearch,
      selectProblem
    }
  }))
  const editor = createTestcaseEditorViewModel({
    selectedProblemId,
    isLoadingProblem: problemDetailResource.isLoadingDetail,
    problemErrorMessage: problemDetailResource.detailErrorMessage,
    problemDetail: problemDetailResource.selectedProblemDetail,
    busySection,
    canUploadTestcaseZip: uploadActions.canUploadTestcaseZip,
    isUploadingTestcaseZip: uploadActions.isUploadingTestcaseZip,
    canCreateTestcase: uploadActions.canCreateTestcase,
    isCreatingTestcase: uploadActions.isCreatingTestcase,
    canViewSpecificTestcase: draft.canViewSpecificTestcase,
    isLoadingTestcases: testcaseListResource.isLoadingTestcases,
    isLoadingSelectedTestcase: selectedTestcaseResource.isLoadingSelectedTestcase,
    testcaseItems: testcaseListResource.testcaseItems,
    testcaseErrorMessage: testcaseListResource.testcaseErrorMessage,
    selectedTestcaseErrorMessage: selectedTestcaseResource.selectedTestcaseErrorMessage,
    selectedTestcaseOrder: draft.selectedTestcaseOrder,
    selectedTestcase: selectedTestcaseResource.selectedTestcase,
    canDeleteSelectedTestcase: uploadActions.canDeleteSelectedTestcase,
    isDeletingSelectedTestcase: uploadActions.isDeletingSelectedTestcase,
    canMoveTestcases: reorderActions.canMoveTestcases,
    isMovingTestcase: reorderActions.isMovingTestcase,
    canSaveSelectedTestcase: draft.canSaveSelectedTestcase,
    isSavingSelectedTestcase: uploadActions.isSavingSelectedTestcase,
    formatCount,
    describeTestcaseContent,
    isLastTestcase,
    setTestcaseSummaryElement,
    testcaseZipInputKey: draft.testcaseZipInputKey,
    selectedTestcaseZipName: draft.selectedTestcaseZipName,
    newTestcaseInput: draft.newTestcaseInput,
    newTestcaseOutput: draft.newTestcaseOutput,
    viewTestcaseOrderInput: draft.viewTestcaseOrderInput,
    selectedTestcaseInputDraft: draft.selectedTestcaseInputDraft,
    selectedTestcaseOutputDraft: draft.selectedTestcaseOutputDraft,
    handleTestcaseZipFileChange: draft.handleTestcaseZipFileChange,
    handleUploadTestcaseZip: uploadActions.handleUploadTestcaseZip,
    handleCreateTestcase: uploadActions.handleCreateTestcase,
    selectTestcase,
    handleDeleteSelectedTestcase: uploadActions.handleDeleteSelectedTestcase,
    handleMoveTestcase: reorderActions.handleMoveTestcase,
    handleSaveSelectedTestcase: uploadActions.handleSaveSelectedTestcase,
    handleViewSelectedTestcase
  })

  return {
    shell,
    toolbar,
    sidebar,
    editor
  }
}
