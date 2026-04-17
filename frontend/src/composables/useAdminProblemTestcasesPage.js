import { computed, ref } from 'vue'
import { useRoute, useRouter } from 'vue-router'

import { useTestcaseDraftState } from '@/composables/adminProblemTestcases/useTestcaseDraftState'
import { useTestcaseMutationState } from '@/composables/adminProblemTestcases/useTestcaseMutationState'
import { useSelectedTestcaseResource } from '@/composables/adminProblemTestcases/useSelectedTestcaseResource'
import { useTestcaseSelectionState } from '@/composables/adminProblemTestcases/useTestcaseSelectionState'
import { useTestcaseListResource } from '@/composables/adminProblemTestcases/useTestcaseListResource'
import { useTestcaseUploadState } from '@/composables/adminProblemTestcases/useTestcaseUploadState'
import {
  normalizeAdminProblemId,
  sanitizeNumericInputValue
} from '@/composables/adminShared/adminProblemSelectionHelpers'
import {
  resetAdminProblemSelectionPageState,
  useAdminProblemSelectionPageShell,
  useAdminProblemSelectionPageWorkspace
} from '@/composables/adminShared/useAdminProblemSelectionPageState'
import { useAdminProblemSelectionWorkspaceCore } from '@/composables/adminShared/useAdminProblemSelectionWorkspace'
import { authStore } from '@/stores/auth/authStore'
import { noticeStore } from '@/stores/notice/noticeStore'
import { formatCount } from '@/utils/numberFormat'

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
      viewTestcaseOrderInput.value = sanitizeNumericInputValue(value)
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
  const canAccessTestcasePage = computed(() => (
    authState.initialized &&
    !authState.isInitializing &&
    isAuthenticated.value &&
    canManageProblems.value
  ))
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
  const testcaseDraftState = useTestcaseDraftState({
    authState,
    busySection,
    selectedTestcaseResource,
    showErrorNotice
  })
  const selectionState = useTestcaseSelectionState({
    authState,
    canAccessPage: canAccessTestcasePage,
    selectedProblemId,
    testcaseListResource,
    selectedTestcaseResource,
    showErrorNotice
  })

  async function resetSelectedProblemState(){
    problemDetailResource.resetSelectedProblemDetail()
    testcaseListResource.resetTestcaseList()
    selectionState.resetSelectedTestcaseState()
    testcaseDraftState.resetDrafts()
  }

  async function loadSelectedProblemData(problemId = selectedProblemId.value, detailResource = problemDetailResource){
    const normalizedProblemId = normalizeAdminProblemId(problemId)

    if (!normalizedProblemId) {
      detailResource.resetSelectedProblemDetail()
      testcaseListResource.resetTestcaseList()
      selectionState.resetSelectedTestcaseState()
      testcaseDraftState.resetDrafts()
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
  const testcaseMutationState = useTestcaseMutationState({
    authState,
    busySection,
    selectedProblemId,
    draftState: testcaseDraftState,
    selectionState,
    testcaseListResource,
    selectedTestcaseResource,
    problemDetailResource,
    showErrorNotice,
    showSuccessNotice
  })
  const testcaseUploadState = useTestcaseUploadState({
    authState,
    busySection,
    formatCount,
    selectedProblemId,
    testcaseZipFile: testcaseDraftState.testcaseZipFile,
    resetZipSelection: testcaseDraftState.resetZipSelection,
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
    titleSearchInputId: 'admin-testcase-problem-search',
    problemIdSearchInputId: 'admin-testcase-problem-id-search'
  })
  const editor = createTestcaseEditorViewModel({
    selectedProblemId,
    isLoadingProblem: problemDetailResource.isLoadingDetail,
    problemErrorMessage: problemDetailResource.detailErrorMessage,
    problemDetail: problemDetailResource.selectedProblemDetail,
    busySection,
    canUploadTestcaseZip: testcaseUploadState.canUploadTestcaseZip,
    isUploadingTestcaseZip: testcaseUploadState.isUploadingTestcaseZip,
    canCreateTestcase: testcaseMutationState.canCreateTestcase,
    isCreatingTestcase: testcaseMutationState.isCreatingTestcase,
    canViewSpecificTestcase: selectionState.canViewSpecificTestcase,
    isLoadingTestcases: testcaseListResource.isLoadingTestcases,
    isLoadingSelectedTestcase: selectedTestcaseResource.isLoadingSelectedTestcase,
    testcaseItems: testcaseListResource.testcaseItems,
    testcaseErrorMessage: testcaseListResource.testcaseErrorMessage,
    selectedTestcaseErrorMessage: selectedTestcaseResource.selectedTestcaseErrorMessage,
    selectedTestcaseOrder: selectionState.selectedTestcaseOrder,
    selectedTestcase: selectedTestcaseResource.selectedTestcase,
    canDeleteSelectedTestcase: testcaseMutationState.canDeleteSelectedTestcase,
    isDeletingSelectedTestcase: testcaseMutationState.isDeletingSelectedTestcase,
    canMoveTestcases: testcaseMutationState.canMoveTestcases,
    isMovingTestcase: testcaseMutationState.isMovingTestcase,
    canSaveSelectedTestcase: testcaseDraftState.canSaveSelectedTestcase,
    isSavingSelectedTestcase: testcaseMutationState.isSavingSelectedTestcase,
    formatCount,
    describeTestcaseContent,
    isLastTestcase,
    setTestcaseSummaryElement: selectionState.setTestcaseSummaryElement,
    testcaseZipInputKey: testcaseDraftState.testcaseZipInputKey,
    selectedTestcaseZipName: testcaseDraftState.selectedTestcaseZipName,
    newTestcaseInput: testcaseDraftState.newTestcaseInput,
    newTestcaseOutput: testcaseDraftState.newTestcaseOutput,
    viewTestcaseOrderInput: selectionState.viewTestcaseOrderInput,
    selectedTestcaseInputDraft: testcaseDraftState.selectedTestcaseInputDraft,
    selectedTestcaseOutputDraft: testcaseDraftState.selectedTestcaseOutputDraft,
    handleTestcaseZipFileChange: testcaseDraftState.handleTestcaseZipFileChange,
    handleUploadTestcaseZip: testcaseUploadState.handleUploadTestcaseZip,
    handleCreateTestcase: testcaseMutationState.handleCreateTestcase,
    selectTestcase: selectionState.selectTestcase,
    handleDeleteSelectedTestcase: testcaseMutationState.handleDeleteSelectedTestcase,
    handleMoveTestcase: testcaseMutationState.handleMoveTestcase,
    handleSaveSelectedTestcase: testcaseMutationState.handleSaveSelectedTestcase,
    handleViewSelectedTestcase: selectionState.handleViewSelectedTestcase
  })

  return {
    shell,
    toolbar,
    sidebar,
    editor
  }
}
