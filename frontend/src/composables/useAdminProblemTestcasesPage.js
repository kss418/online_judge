import { computed, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'

import { useTestcaseEditorActions } from '@/composables/adminProblemTestcases/useTestcaseEditorActions'
import { useSelectedTestcaseResource } from '@/composables/adminProblemTestcases/useSelectedTestcaseResource'
import { useTestcaseSelectionState } from '@/composables/adminProblemTestcases/useTestcaseSelectionState'
import { formatProblemLimit } from '@/composables/adminProblemTestcases/testcaseHelpers'
import { useTestcaseListResource } from '@/composables/adminProblemTestcases/useTestcaseListResource'
import {
  resetAdminProblemSelectionPageState,
  useAdminProblemSelectionPageShell,
  useAdminProblemSelectionPageWorkspace
} from '@/composables/adminShared/useAdminProblemSelectionPageState'
import { useAdminProblemSelectionWorkspaceCore } from '@/composables/adminShared/useAdminProblemSelectionWorkspace'
import { useTestcaseZipInput } from '@/composables/adminShared/useTestcaseZipInput'
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
  const newTestcaseInput = ref('')
  const newTestcaseOutput = ref('')
  const selectedTestcaseInputDraft = ref('')
  const selectedTestcaseOutputDraft = ref('')
  const {
    testcaseZipFile,
    testcaseZipInputKey,
    selectedTestcaseZipName,
    resetTestcaseZipSelection,
    handleTestcaseZipFileChange
  } = useTestcaseZipInput({
    onInvalidZip(){
      showErrorNotice('ZIP 파일만 업로드할 수 있습니다.')
    }
  })
  const canAccessTestcasePage = computed(() => (
    authState.initialized &&
    !authState.isInitializing &&
    isAuthenticated.value &&
    canManageProblems.value
  ))

  function resetTestcaseDraftState(){
    newTestcaseInput.value = ''
    newTestcaseOutput.value = ''
    selectedTestcaseInputDraft.value = ''
    selectedTestcaseOutputDraft.value = ''
    resetTestcaseZipSelection()
  }
  const workspaceCore = useAdminProblemSelectionWorkspaceCore({
    route,
    router,
    routeName: 'admin-problem-testcases',
    authState,
    canManageProblems,
    showErrorNotice,
    formatCount
  })

  const selectedProblemId = workspaceCore.selectedProblemId
  const problemCatalogResource = workspaceCore.problemCatalogResource
  const problemDetailResource = workspaceCore.problemDetailResource
  const testcaseListResource = useTestcaseListResource({
    authState,
    selectedProblemId
  })
  const selectedTestcaseResource = useSelectedTestcaseResource({
    authState,
    selectedProblemId
  })
  const selectionState = useTestcaseSelectionState({
    authState,
    canAccessPage: canAccessTestcasePage,
    selectedProblemId,
    testcaseListResource,
    selectedTestcaseResource,
    showErrorNotice
  })
  const canSaveSelectedTestcase = computed(() => {
    if (!selectedTestcaseResource.selectedTestcase.value || !authState.token || busySection.value) {
      return false
    }

    return (
      selectedTestcaseInputDraft.value !== selectedTestcaseResource.selectedTestcase.value.testcase_input ||
      selectedTestcaseOutputDraft.value !== selectedTestcaseResource.selectedTestcase.value.testcase_output
    )
  })

  async function resetSelectedProblemState(){
    problemDetailResource.resetSelectedProblemDetail()
    testcaseListResource.resetTestcaseList()
    selectionState.resetSelectedTestcaseState()
    resetTestcaseDraftState()
  }

  async function loadSelectedProblemData(problemId = selectedProblemId.value, detailResource = problemDetailResource){
    const normalizedProblemId = parsePositiveInteger(problemId)

    if (normalizedProblemId == null) {
      detailResource.resetSelectedProblemDetail()
      testcaseListResource.resetTestcaseList()
      selectionState.resetSelectedTestcaseState()
      resetTestcaseDraftState()
      return {
        status: 'reset'
      }
    }

    const [problemResult, testcaseResult] = await Promise.all([
      detailResource.loadProblemDetail(normalizedProblemId),
      selectionState.loadTestcases()
    ])

    return problemResult?.status === 'error' ? problemResult : testcaseResult
  }

  async function resetPageState(){
    await resetAdminProblemSelectionPageState({
      workspaceCore,
      busySection,
      resetSelectedProblemState
    })
  }

  const workspace = useAdminProblemSelectionPageWorkspace({
    core: workspaceCore,
    authState,
    initializeAuth,
    isAuthenticated,
    canManageProblems,
    accessMessages: {
      loggedOutMessage: '테스트케이스 관리 페이지는 로그인한 관리자만 사용할 수 있습니다.',
      deniedMessage: '이 페이지는 관리자만 접근할 수 있습니다.'
    },
    canRefresh: () => !busySection.value,
    resetSelectedProblemState,
    loadSelectedProblemData,
    resetPageState
  })
  const testcaseEditorActions = useTestcaseEditorActions({
    authState,
    busySection,
    formatCount,
    selectedProblemId,
    newTestcaseInput,
    newTestcaseOutput,
    selectedTestcaseInputDraft,
    selectedTestcaseOutputDraft,
    testcaseZipFile,
    resetTestcaseZipSelection,
    canSaveSelectedTestcase,
    selectionState,
    testcaseListResource,
    selectedTestcaseResource,
    problemDetailResource,
    reloadProblems: workspaceCore.loadProblems,
    reloadSelectedProblemData: loadSelectedProblemData,
    showErrorNotice,
    showSuccessNotice
  })

  const toolbarStatusLabel = computed(() => {
    if (
      workspace.isLoadingProblems.value ||
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
    selectedTestcaseInputDraft.value = testcase?.testcase_input ?? ''
    selectedTestcaseOutputDraft.value = testcase?.testcase_output ?? ''
  }, {
    immediate: true
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

  const {
    shell,
    toolbar,
    sidebar
  } = useAdminProblemSelectionPageShell({
    workspace,
    canManageProblems,
    busySection,
    statusLabel: toolbarStatusLabel,
    statusTone: toolbarStatusTone,
    toolbarExtraModel: {
      isLoadingProblem: problemDetailResource.isLoadingDetail,
      isLoadingTestcases: testcaseListResource.isLoadingTestcases,
      selectedProblemId
    },
    formatCount,
    formatProblemLimit,
    titleSearchInputId: 'admin-testcase-problem-search',
    problemIdSearchInputId: 'admin-testcase-problem-id-search'
  })
  const editor = createTestcaseEditorViewModel({
    selectedProblemId,
    isLoadingProblem: problemDetailResource.isLoadingDetail,
    problemErrorMessage: problemDetailResource.detailErrorMessage,
    problemDetail: problemDetailResource.selectedProblemDetail,
    busySection,
    canUploadTestcaseZip: testcaseEditorActions.canUploadTestcaseZip,
    isUploadingTestcaseZip: testcaseEditorActions.isUploadingTestcaseZip,
    canCreateTestcase: testcaseEditorActions.canCreateTestcase,
    isCreatingTestcase: testcaseEditorActions.isCreatingTestcase,
    canViewSpecificTestcase: selectionState.canViewSpecificTestcase,
    isLoadingTestcases: testcaseListResource.isLoadingTestcases,
    isLoadingSelectedTestcase: selectedTestcaseResource.isLoadingSelectedTestcase,
    testcaseItems: testcaseListResource.testcaseItems,
    testcaseErrorMessage: testcaseListResource.testcaseErrorMessage,
    selectedTestcaseErrorMessage: selectedTestcaseResource.selectedTestcaseErrorMessage,
    selectedTestcaseOrder: selectionState.selectedTestcaseOrder,
    selectedTestcase: selectedTestcaseResource.selectedTestcase,
    canDeleteSelectedTestcase: testcaseEditorActions.canDeleteSelectedTestcase,
    isDeletingSelectedTestcase: testcaseEditorActions.isDeletingSelectedTestcase,
    canMoveTestcases: testcaseEditorActions.canMoveTestcases,
    isMovingTestcase: testcaseEditorActions.isMovingTestcase,
    canSaveSelectedTestcase,
    isSavingSelectedTestcase: testcaseEditorActions.isSavingSelectedTestcase,
    formatCount,
    describeTestcaseContent,
    isLastTestcase,
    setTestcaseSummaryElement: selectionState.setTestcaseSummaryElement,
    testcaseZipInputKey,
    selectedTestcaseZipName,
    newTestcaseInput,
    newTestcaseOutput,
    viewTestcaseOrderInput: selectionState.viewTestcaseOrderInput,
    selectedTestcaseInputDraft,
    selectedTestcaseOutputDraft,
    handleTestcaseZipFileChange,
    handleUploadTestcaseZip: testcaseEditorActions.handleUploadTestcaseZip,
    handleCreateTestcase: testcaseEditorActions.handleCreateTestcase,
    selectTestcase: selectionState.selectTestcase,
    handleDeleteSelectedTestcase: testcaseEditorActions.handleDeleteSelectedTestcase,
    handleMoveTestcase: testcaseEditorActions.handleMoveTestcase,
    handleSaveSelectedTestcase: testcaseEditorActions.handleSaveSelectedTestcase,
    handleViewSelectedTestcase: selectionState.handleViewSelectedTestcase
  })

  return {
    shell,
    toolbar,
    sidebar,
    editor
  }
}
