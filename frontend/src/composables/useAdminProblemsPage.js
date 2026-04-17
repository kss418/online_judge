import { computed, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'

import { useProblemActionFeedback } from '@/composables/adminProblems/useProblemActionFeedback'
import { useProblemDetailEditorState } from '@/composables/adminProblems/useProblemDetailEditorState'
import { useProblemLifecycleState } from '@/composables/adminProblems/useProblemLifecycleState'
import { formatProblemLimit } from '@/composables/adminProblems/problemHelpers'
import { useProblemSampleEditorState } from '@/composables/adminProblems/useProblemSampleEditorState'
import {
  resetAdminProblemSelectionPageState,
  useAdminProblemSelectionPageShell,
  useAdminProblemSelectionPageWorkspace
} from '@/composables/adminShared/useAdminProblemSelectionPageState'
import { useAdminProblemSelectionWorkspaceCore } from '@/composables/adminShared/useAdminProblemSelectionWorkspace'
import { authStore } from '@/stores/auth/authStore'
import { noticeStore } from '@/stores/notice/noticeStore'
import { formatCount } from '@/utils/numberFormat'
import { parsePositiveInteger } from '@/utils/parse'

function createProblemEditorViewModel({
  isLoadingDetail,
  detailErrorMessage,
  selectedProblemDetail,
  busySection,
  canSaveTitle,
  canSaveLimits,
  canSaveStatement,
  canCreateSample,
  canUploadTestcaseZip,
  canDeleteLastSample,
  isSavingTitle,
  isSavingLimits,
  isSavingStatement,
  isCreatingSample,
  isDeletingLastSample,
  isUploadingTestcaseZip,
  titleDraft,
  timeLimitDraft,
  memoryLimitDraft,
  descriptionDraft,
  inputFormatDraft,
  outputFormatDraft,
  noteDraft,
  sampleDrafts,
  testcaseZipInputKey,
  selectedTestcaseZipName,
  formatCount,
  isSavingSample,
  canSaveSample,
  isLastSample,
  handleSaveTitle,
  handleSaveLimits,
  handleSaveStatement,
  handleCreateSample,
  handleSaveSample,
  handleDeleteLastSample,
  handleTestcaseZipFileChange,
  handleUploadTestcaseZip,
  openRejudgeDialog,
  openDeleteDialog
}){
  const actions = {
    updateTitleDraft(value){
      titleDraft.value = value
    },
    updateTimeLimitDraft(value){
      timeLimitDraft.value = value
    },
    updateMemoryLimitDraft(value){
      memoryLimitDraft.value = value
    },
    updateDescriptionDraft(value){
      descriptionDraft.value = value
    },
    updateInputFormatDraft(value){
      inputFormatDraft.value = value
    },
    updateOutputFormatDraft(value){
      outputFormatDraft.value = value
    },
    updateNoteDraft(value){
      noteDraft.value = value
    },
    saveTitle: handleSaveTitle,
    saveLimits: handleSaveLimits,
    saveStatement: handleSaveStatement,
    createSample: handleCreateSample,
    saveSample: handleSaveSample,
    deleteLastSample: handleDeleteLastSample,
    changeTestcaseZip: handleTestcaseZipFileChange,
    uploadTestcaseZip: handleUploadTestcaseZip,
    openRejudgeDialog,
    openDeleteDialog
  }

  return computed(() => {
    const shell = isLoadingDetail.value
      ? {
        mode: 'loading',
        message: '문제 정보를 불러오는 중입니다.'
      }
      : detailErrorMessage.value
        ? {
          mode: 'error',
          message: detailErrorMessage.value
        }
        : !selectedProblemDetail.value
          ? {
            mode: 'empty',
            message: '왼쪽 목록에서 문제를 선택하거나 새 문제를 생성하세요.'
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
            selectedProblemDetail: selectedProblemDetail.value,
            busySection: busySection.value,
            formatCount
          },
          actions: {
            openRejudgeDialog: actions.openRejudgeDialog
          }
        },
        basics: {
          model: {
            titleDraft: titleDraft.value,
            timeLimitDraft: timeLimitDraft.value,
            memoryLimitDraft: memoryLimitDraft.value,
            canSaveTitle: canSaveTitle.value,
            canSaveLimits: canSaveLimits.value,
            isSavingTitle: isSavingTitle.value,
            isSavingLimits: isSavingLimits.value
          },
          actions: {
            updateTitleDraft: actions.updateTitleDraft,
            updateTimeLimitDraft: actions.updateTimeLimitDraft,
            updateMemoryLimitDraft: actions.updateMemoryLimitDraft,
            saveTitle: actions.saveTitle,
            saveLimits: actions.saveLimits
          }
        },
        statement: {
          model: {
            descriptionDraft: descriptionDraft.value,
            inputFormatDraft: inputFormatDraft.value,
            outputFormatDraft: outputFormatDraft.value,
            noteDraft: noteDraft.value,
            canSaveStatement: canSaveStatement.value,
            isSavingStatement: isSavingStatement.value
          },
          actions: {
            updateDescriptionDraft: actions.updateDescriptionDraft,
            updateInputFormatDraft: actions.updateInputFormatDraft,
            updateOutputFormatDraft: actions.updateOutputFormatDraft,
            updateNoteDraft: actions.updateNoteDraft,
            saveStatement: actions.saveStatement
          }
        },
        samples: {
          model: {
            sampleDrafts: sampleDrafts.value,
            canCreateSample: canCreateSample.value,
            canDeleteLastSample: canDeleteLastSample.value,
            isCreatingSample: isCreatingSample.value,
            isDeletingLastSample: isDeletingLastSample.value,
            formatCount,
            isSavingSample,
            canSaveSample,
            isLastSample
          },
          actions: {
            createSample: actions.createSample,
            saveSample: actions.saveSample,
            deleteLastSample: actions.deleteLastSample
          }
        },
        testcaseUpload: {
          model: {
            testcaseZipInputKey: testcaseZipInputKey.value,
            selectedTestcaseZipName: selectedTestcaseZipName.value,
            busySection: busySection.value,
            canUploadTestcaseZip: canUploadTestcaseZip.value,
            isUploadingTestcaseZip: isUploadingTestcaseZip.value
          },
          actions: {
            changeTestcaseZip: actions.changeTestcaseZip,
            uploadTestcaseZip: actions.uploadTestcaseZip
          }
        },
        danger: {
          model: {
            busySection: busySection.value
          },
          actions: {
            openDeleteDialog: actions.openDeleteDialog
          }
        }
      }
    }
  })
}

function createProblemConfirmDialogViewModel({
  open,
  problemIdInput,
  titleInput,
  selectedProblemDetail,
  isBusy,
  canConfirm,
  confirmLabel,
  titleId,
  kicker,
  title,
  description,
  summaryCopy = '',
  confirmButtonClass = '',
  close,
  confirm
}){
  const actions = {
    updateProblemIdInput(value){
      problemIdInput.value = value
    },
    updateTitleInput(value){
      titleInput.value = value
    },
    close,
    confirm
  }

  return computed(() => ({
    model: {
      open: open.value,
      titleId,
      kicker,
      title,
      description,
      summaryLabel: selectedProblemDetail.value
        ? `#${formatCount(selectedProblemDetail.value.problem_id)} ${selectedProblemDetail.value.title}`
        : '',
      summaryCopy,
      problemIdInput: problemIdInput.value,
      problemIdPlaceholder: selectedProblemDetail.value
        ? String(selectedProblemDetail.value.problem_id)
        : '예: 1000',
      titleInput: titleInput.value,
      titlePlaceholder: selectedProblemDetail.value?.title || '문제 제목',
      isBusy: isBusy.value,
      canConfirm: canConfirm.value,
      confirmLabel: confirmLabel.value,
      confirmButtonClass
    },
    actions
  }))
}

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
  const workspaceCore = useAdminProblemSelectionWorkspaceCore({
    route,
    router,
    routeName: 'admin-problems',
    authState,
    canManageProblems,
    showErrorNotice,
    formatCount
  })

  const selectedProblemId = workspaceCore.selectedProblemId
  const problemListResource = workspaceCore.problemCatalogResource
  const problemDetailResource = workspaceCore.problemDetailResource
  const problemActionFeedback = useProblemActionFeedback({
    selectedProblemDetail: problemDetailResource.selectedProblemDetail,
    busySection
  })
  const problemDetailEditorState = useProblemDetailEditorState({
    authState,
    busySection,
    selectedProblemDetail: problemDetailResource.selectedProblemDetail,
    problemDetailResource,
    problemCatalogResource: problemListResource,
    problemActionFeedback,
    formatCount
  })
  const problemSampleEditorState = useProblemSampleEditorState({
    authState,
    busySection,
    formatCount,
    selectedProblemDetail: problemDetailResource.selectedProblemDetail,
    problemDetailResource,
    problemActionFeedback,
    loadSelectedProblem
  })

  async function loadSelectedProblem(problemId = selectedProblemId.value, options = {}, detailResource = problemDetailResource){
    const normalizedProblemId = parsePositiveInteger(problemId)
    if (normalizedProblemId == null) {
      detailResource.resetSelectedProblemDetail()
      problemDetailEditorState.reset()
      problemSampleEditorState.reset()
      return {
        status: 'reset'
      }
    }

    problemActionFeedback.clearActionError()
    problemDetailEditorState.reset()
    problemSampleEditorState.reset({
      skipTestcaseZipReset: options.skipTestcaseZipReset === true
    })

    const result = await detailResource.loadProblemDetail(normalizedProblemId)

    if (result.status !== 'success') {
      return result
    }

    problemDetailEditorState.assignFromProblemDetail(result.data)
    problemSampleEditorState.assignSamples(result.data.samples)
    return result
  }

  async function handleCreatedProblem(problemId){
    const hadAppliedSearch = workspaceCore.query.hasAppliedSearch.value
    const didNavigate = await workspaceCore.query.selectCreatedProblem(problemId)

    if (didNavigate && !hadAppliedSearch) {
      await workspaceCore.loadProblems({
        preferredProblemId: problemId
      })
    }
  }

  async function resetSelectedProblemState(){
    problemDetailResource.resetSelectedProblemDetail()
    problemDetailEditorState.reset()
    problemSampleEditorState.reset()
  }

  async function loadSelectedProblemData(problemId = selectedProblemId.value){
    return loadSelectedProblem(problemId)
  }

  async function resetPageState(){
    problemLifecycleState.newProblemTitle.value = ''
    await resetAdminProblemSelectionPageState({
      workspaceCore,
      busySection,
      resetSelectedProblemState
    })
    problemActionFeedback.resetActionState()
  }

  const problemLifecycleState = useProblemLifecycleState({
    authState,
    busySection,
    formatCount,
    problemActionFeedback,
    selectedProblemDetail: problemDetailResource.selectedProblemDetail,
    loadProblems: workspaceCore.loadProblems,
    onCreatedProblem: handleCreatedProblem
  })

  const workspace = useAdminProblemSelectionPageWorkspace({
    core: workspaceCore,
    authState,
    initializeAuth,
    isAuthenticated,
    canManageProblems,
    accessMessages: {
      loggedOutMessage: '문제 관리 페이지는 로그인한 관리자만 사용할 수 있습니다.',
      deniedMessage: '이 페이지는 관리자만 접근할 수 있습니다.'
    },
    resetSelectedProblemState,
    loadSelectedProblemData,
    resetPageState
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

  const toolbarStatusLabel = computed(() =>
    workspace.isLoadingProblems.value ? 'Loading' : `${problemListResource.problemCount.value} Problems`
  )
  const toolbarStatusTone = computed(() =>
    problemListResource.listErrorMessage.value ? 'danger' : 'success'
  )
  const rejudgeConfirmLabel = computed(() => (
    problemLifecycleState.isRejudgingProblem.value ? '요청 중...' : '재채점 확정'
  ))
  const deleteConfirmLabel = computed(() => (
    problemLifecycleState.isDeletingProblem.value ? '삭제 중...' : '삭제 확정'
  ))

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
    formatCount,
    formatProblemLimit,
    titleSearchInputId: 'admin-problem-title-search',
    problemIdSearchInputId: 'admin-problem-id-search',
    sidebarCreate: {
      model: {
        newProblemTitle: problemLifecycleState.newProblemTitle,
        canCreateProblem: problemLifecycleState.canCreateProblem,
        isCreatingProblem: problemLifecycleState.isCreatingProblem
      },
      actions: {
        updateNewProblemTitle: problemLifecycleState.updateNewProblemTitle,
        createProblem: problemLifecycleState.handleCreateProblem
      }
    }
  })
  const editor = createProblemEditorViewModel({
    isLoadingDetail: problemDetailResource.isLoadingDetail,
    detailErrorMessage: problemDetailResource.detailErrorMessage,
    selectedProblemDetail: problemDetailResource.selectedProblemDetail,
    busySection,
    canSaveTitle: problemDetailEditorState.canSaveTitle,
    canSaveLimits: problemDetailEditorState.canSaveLimits,
    canSaveStatement: problemDetailEditorState.canSaveStatement,
    canCreateSample: problemSampleEditorState.canCreateSample,
    canUploadTestcaseZip: problemSampleEditorState.canUploadTestcaseZip,
    canDeleteLastSample: problemSampleEditorState.canDeleteLastSample,
    isSavingTitle: problemDetailEditorState.isSavingTitle,
    isSavingLimits: problemDetailEditorState.isSavingLimits,
    isSavingStatement: problemDetailEditorState.isSavingStatement,
    isCreatingSample: problemSampleEditorState.isCreatingSample,
    isDeletingLastSample: problemSampleEditorState.isDeletingLastSample,
    isUploadingTestcaseZip: problemSampleEditorState.isUploadingTestcaseZip,
    titleDraft: problemDetailEditorState.titleDraft,
    timeLimitDraft: problemDetailEditorState.timeLimitDraft,
    memoryLimitDraft: problemDetailEditorState.memoryLimitDraft,
    descriptionDraft: problemDetailEditorState.descriptionDraft,
    inputFormatDraft: problemDetailEditorState.inputFormatDraft,
    outputFormatDraft: problemDetailEditorState.outputFormatDraft,
    noteDraft: problemDetailEditorState.noteDraft,
    sampleDrafts: problemSampleEditorState.sampleDrafts,
    testcaseZipInputKey: problemSampleEditorState.testcaseZipInputKey,
    selectedTestcaseZipName: problemSampleEditorState.selectedTestcaseZipName,
    formatCount,
    isSavingSample: problemSampleEditorState.isSavingSample,
    canSaveSample: problemSampleEditorState.canSaveSample,
    isLastSample: problemSampleEditorState.isLastSample,
    handleSaveTitle: problemDetailEditorState.handleSaveTitle,
    handleSaveLimits: problemDetailEditorState.handleSaveLimits,
    handleSaveStatement: problemDetailEditorState.handleSaveStatement,
    handleCreateSample: problemSampleEditorState.handleCreateSample,
    handleSaveSample: problemSampleEditorState.handleSaveSample,
    handleDeleteLastSample: problemSampleEditorState.handleDeleteLastSample,
    handleTestcaseZipFileChange: problemSampleEditorState.handleTestcaseZipFileChange,
    handleUploadTestcaseZip: problemSampleEditorState.handleUploadTestcaseZip,
    openRejudgeDialog: problemActionFeedback.openRejudgeDialog,
    openDeleteDialog: problemActionFeedback.openDeleteDialog
  })
  const rejudgeDialog = createProblemConfirmDialogViewModel({
    open: problemActionFeedback.rejudgeDialogOpen,
    problemIdInput: problemActionFeedback.rejudgeConfirmProblemId,
    titleInput: problemActionFeedback.rejudgeConfirmTitle,
    selectedProblemDetail: problemDetailResource.selectedProblemDetail,
    isBusy: problemLifecycleState.isRejudgingProblem,
    canConfirm: problemActionFeedback.canRejudgeSelectedProblem,
    confirmLabel: rejudgeConfirmLabel,
    titleId: 'admin-problem-rejudge-title',
    kicker: 'confirm rejudge',
    title: '문제 재채점 확인',
    description: '아래 두 값을 모두 정확히 다시 입력해야 재채점을 요청할 수 있습니다.',
    summaryCopy: '현재 문제의 `accepted`, `wrong_answer` 제출을 다시 채점 대기열에 넣습니다.',
    confirmButtonClass: 'admin-problem-rejudge-confirm',
    close: problemActionFeedback.closeRejudgeDialog,
    confirm: problemLifecycleState.handleRejudgeProblem
  })
  const deleteDialog = createProblemConfirmDialogViewModel({
    open: problemActionFeedback.deleteDialogOpen,
    problemIdInput: problemActionFeedback.deleteConfirmProblemId,
    titleInput: problemActionFeedback.deleteConfirmTitle,
    selectedProblemDetail: problemDetailResource.selectedProblemDetail,
    isBusy: problemLifecycleState.isDeletingProblem,
    canConfirm: problemActionFeedback.canDeleteSelectedProblem,
    confirmLabel: deleteConfirmLabel,
    titleId: 'admin-problem-delete-title',
    kicker: 'confirm delete',
    title: '문제 삭제 확인',
    description: '아래 두 값을 모두 정확히 다시 입력해야 삭제할 수 있습니다.',
    confirmButtonClass: 'admin-problem-delete-confirm',
    close: problemActionFeedback.closeDeleteDialog,
    confirm: problemLifecycleState.handleDeleteProblem
  })
  const dialogs = computed(() => ({
    rejudge: rejudgeDialog.value,
    delete: deleteDialog.value
  }))

  return {
    shell,
    toolbar,
    sidebar,
    editor,
    dialogs
  }
}
