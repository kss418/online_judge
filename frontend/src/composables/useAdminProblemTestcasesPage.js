import { computed, ref } from 'vue'
import { useRoute, useRouter } from 'vue-router'

import { useTestcaseListDetailMutationState } from '@/composables/adminProblemTestcases/useTestcaseListDetailMutationState'
import { useSelectedTestcaseResource } from '@/composables/adminProblemTestcases/useSelectedTestcaseResource'
import { useTestcaseSelectionSyncState } from '@/composables/adminProblemTestcases/useTestcaseSelectionSyncState'
import { useTestcaseListResource } from '@/composables/adminProblemTestcases/useTestcaseListResource'
import {
  sanitizeNumericInputValue
} from '@/composables/adminShared/adminProblemSelectionHelpers'
import {
  useAdminProblemSelectionPageShell,
  useAdminProblemSelectionPageState
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
  const selectionState = useTestcaseSelectionSyncState({
    authState,
    canAccessPage: canAccessTestcasePage,
    selectedProblemId,
    testcaseListResource,
    selectedTestcaseResource,
    showErrorNotice
  })

  async function reloadSelectedProblemData(...args){
    return pageState.loadSelectedProblemData(...args)
  }

  const testcaseListDetailMutationState = useTestcaseListDetailMutationState({
    authState,
    busySection,
    formatCount,
    selectedProblemId,
    problemDetailResource,
    testcaseListResource,
    selectedTestcaseResource,
    selectionState,
    reloadProblems: workspaceCore.loadProblems,
    reloadSelectedProblemData,
    showErrorNotice,
    showSuccessNotice
  })

  const pageState = useAdminProblemSelectionPageState({
    core: workspaceCore,
    authState,
    initializeAuth,
    isAuthenticated,
    canManageProblems,
    accessMessages: {
      loggedOutMessage: '테스트케이스 관리 페이지는 로그인한 관리자만 사용할 수 있습니다.',
      deniedMessage: '이 페이지는 관리자만 접근할 수 있습니다.'
    },
    busySection,
    canRefresh: () => !busySection.value,
    selectionResetters: [
      testcaseListResource.resetTestcaseList,
      selectionState.resetSelectedTestcaseState,
      testcaseListDetailMutationState.reset
    ],
    additionalSelectedProblemLoaders: [
      () => selectionState.loadTestcases()
    ]
  })
  const workspace = {
    ...workspaceCore,
    ...pageState
  }

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
    canUploadTestcaseZip: testcaseListDetailMutationState.canUploadTestcaseZip,
    isUploadingTestcaseZip: testcaseListDetailMutationState.isUploadingTestcaseZip,
    canCreateTestcase: testcaseListDetailMutationState.canCreateTestcase,
    isCreatingTestcase: testcaseListDetailMutationState.isCreatingTestcase,
    canViewSpecificTestcase: selectionState.canViewSpecificTestcase,
    isLoadingTestcases: testcaseListResource.isLoadingTestcases,
    isLoadingSelectedTestcase: selectedTestcaseResource.isLoadingSelectedTestcase,
    testcaseItems: testcaseListResource.testcaseItems,
    testcaseErrorMessage: testcaseListResource.testcaseErrorMessage,
    selectedTestcaseErrorMessage: selectedTestcaseResource.selectedTestcaseErrorMessage,
    selectedTestcaseOrder: selectionState.selectedTestcaseOrder,
    selectedTestcase: selectedTestcaseResource.selectedTestcase,
    canDeleteSelectedTestcase: testcaseListDetailMutationState.canDeleteSelectedTestcase,
    isDeletingSelectedTestcase: testcaseListDetailMutationState.isDeletingSelectedTestcase,
    canMoveTestcases: testcaseListDetailMutationState.canMoveTestcases,
    isMovingTestcase: testcaseListDetailMutationState.isMovingTestcase,
    canSaveSelectedTestcase: testcaseListDetailMutationState.canSaveSelectedTestcase,
    isSavingSelectedTestcase: testcaseListDetailMutationState.isSavingSelectedTestcase,
    formatCount,
    describeTestcaseContent,
    isLastTestcase,
    setTestcaseSummaryElement: selectionState.setTestcaseSummaryElement,
    testcaseZipInputKey: testcaseListDetailMutationState.testcaseZipInputKey,
    selectedTestcaseZipName: testcaseListDetailMutationState.selectedTestcaseZipName,
    newTestcaseInput: testcaseListDetailMutationState.newTestcaseInput,
    newTestcaseOutput: testcaseListDetailMutationState.newTestcaseOutput,
    viewTestcaseOrderInput: selectionState.viewTestcaseOrderInput,
    selectedTestcaseInputDraft: testcaseListDetailMutationState.selectedTestcaseInputDraft,
    selectedTestcaseOutputDraft: testcaseListDetailMutationState.selectedTestcaseOutputDraft,
    handleTestcaseZipFileChange: testcaseListDetailMutationState.handleTestcaseZipFileChange,
    handleUploadTestcaseZip: testcaseListDetailMutationState.handleUploadTestcaseZip,
    handleCreateTestcase: testcaseListDetailMutationState.handleCreateTestcase,
    selectTestcase: selectionState.selectTestcase,
    handleDeleteSelectedTestcase: testcaseListDetailMutationState.handleDeleteSelectedTestcase,
    handleMoveTestcase: testcaseListDetailMutationState.handleMoveTestcase,
    handleSaveSelectedTestcase: testcaseListDetailMutationState.handleSaveSelectedTestcase,
    handleViewSelectedTestcase: selectionState.handleViewSelectedTestcase
  })

  return {
    shell,
    toolbar,
    sidebar,
    editor
  }
}
