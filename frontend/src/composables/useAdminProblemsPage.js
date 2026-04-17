import { computed, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'

import { useProblemActionFeedback } from '@/composables/adminProblems/useProblemActionFeedback'
import { useProblemAdminActions } from '@/composables/adminProblems/useProblemAdminActions'
import { useProblemEditorDraft } from '@/composables/adminProblems/useProblemEditorDraft'
import { formatProblemLimit } from '@/composables/adminProblems/problemHelpers'
import { useAdminProblemSelectionWorkspace } from '@/composables/adminShared/useAdminProblemSelectionWorkspace'
import { useAdminProblemSidebarModel } from '@/composables/adminShared/useAdminProblemSidebarModel'
import { useAdminProblemToolbarState } from '@/composables/adminShared/useAdminProblemToolbarState'
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
  const newProblemTitle = ref('')
  let pageSetupReady = false

  async function waitForPageSetup(){
    while (!pageSetupReady) {
      await Promise.resolve()
    }
  }

  function readLegacySelectedProblemId(){
    const queryProblemId = Array.isArray(route.query.problemId)
      ? route.query.problemId[0]
      : route.query.problemId

    return parsePositiveInteger(queryProblemId) ?? 0
  }

  async function canonicalizeLegacySelectedProblemRoute({
    selectedProblemId,
    query
  }){
    if (!Object.prototype.hasOwnProperty.call(route.query, 'problemId')) {
      return false
    }

    await query.replaceSelectedProblem(selectedProblemId.value || readLegacySelectedProblemId())
    return true
  }

  async function resetSelectedProblemStateForWorkspace({
    problemDetailResource
  }){
    await waitForPageSetup()
    problemDetailResource.resetSelectedProblemDetail()
    editorDraft.resetEditorDrafts()
  }

  async function loadSelectedProblemDataForWorkspace({
    problemId,
    problemDetailResource,
    selectedProblemId
  }){
    await waitForPageSetup()

    const normalizedProblemId = parsePositiveInteger(problemId ?? selectedProblemId.value)
    if (normalizedProblemId == null) {
      problemDetailResource.resetSelectedProblemDetail()
      editorDraft.resetEditorDrafts()
      return {
        status: 'reset'
      }
    }

    return loadSelectedProblem(normalizedProblemId, {}, problemDetailResource)
  }

  async function resetPageStateForWorkspace({
    query,
    problemCatalogResource,
    problemDetailResource
  }){
    await waitForPageSetup()
    query.resetSearchControls()
    newProblemTitle.value = ''
    busySection.value = ''
    problemCatalogResource.resetProblems()
    await resetSelectedProblemStateForWorkspace({
      problemDetailResource
    })
    problemActionFeedback.resetActionState()
  }

  const workspace = useAdminProblemSelectionWorkspace({
    route,
    router,
    routeName: 'admin-problems',
    authState,
    initializeAuth,
    isAuthenticated,
    canManageProblems,
    showErrorNotice,
    formatCount,
    accessMessages: {
      loggedOutMessage: '문제 관리 페이지는 로그인한 관리자만 사용할 수 있습니다.',
      deniedMessage: '이 페이지는 관리자만 접근할 수 있습니다.'
    },
    beforeAllowed: canonicalizeLegacySelectedProblemRoute,
    resetSelectedProblemState: resetSelectedProblemStateForWorkspace,
    loadSelectedProblemData: loadSelectedProblemDataForWorkspace,
    resetPageState: resetPageStateForWorkspace
  })

  const selectedProblemId = workspace.selectedProblemId
  const problemListResource = workspace.problemCatalogResource
  const problemDetailResource = workspace.problemDetailResource
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

  async function loadSelectedProblem(problemId = selectedProblemId.value, options = {}, detailResource = problemDetailResource){
    await waitForPageSetup()

    const normalizedProblemId = parsePositiveInteger(problemId)
    if (normalizedProblemId == null) {
      detailResource.resetSelectedProblemDetail()
      editorDraft.resetEditorDrafts()
      return {
        status: 'reset'
      }
    }

    problemActionFeedback.clearActionError()
    editorDraft.resetEditorDrafts({
      skipTestcaseZipReset: options.skipTestcaseZipReset === true
    })

    const result = await detailResource.loadProblemDetail(normalizedProblemId)

    if (result.status !== 'success') {
      return result
    }

    editorDraft.assignEditorDrafts(result.data)
    return result
  }

  async function handleCreatedProblem(problemId){
    const hadAppliedSearch = workspace.query.hasAppliedSearch.value
    const didNavigate = await workspace.query.selectCreatedProblem(problemId)

    if (didNavigate && !hadAppliedSearch) {
      await workspace.loadProblems({
        preferredProblemId: problemId
      })
    }
  }

  function updateNewProblemTitle(value){
    newProblemTitle.value = value
  }

  const loadProblems = workspace.loadProblems

  const problemActions = useProblemAdminActions({
    authState,
    formatCount,
    busySection,
    feedback: problemActionFeedback,
    newProblemTitle,
    selectedProblemDetail: problemDetailResource.selectedProblemDetail,
    updateSelectedProblemDetail: problemDetailResource.setSelectedProblemDetail,
    testcaseZipFile: editorDraft.testcaseZipFile,
    resetTestcaseZipSelection: editorDraft.resetTestcaseZipSelection,
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
    applySelectedProblemVersion: problemDetailResource.applyProblemVersion,
    mergeProblemSummary: problemListResource.mergeProblemSummary,
    loadProblems,
    loadSelectedProblem,
    onCreatedProblem: handleCreatedProblem
  })

  pageSetupReady = true

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
    problemActions.isRejudgingProblem.value ? '요청 중...' : '재채점 확정'
  ))
  const deleteConfirmLabel = computed(() => (
    problemActions.isDeletingProblem.value ? '삭제 중...' : '삭제 확정'
  ))

  const shell = workspace.shell
  const toolbar = useAdminProblemToolbarState({
    workspace,
    canManageProblems,
    busySection,
    statusLabel: toolbarStatusLabel,
    statusTone: toolbarStatusTone
  })
  const sidebar = useAdminProblemSidebarModel({
    workspace,
    busySection,
    formatCount,
    formatProblemLimit,
    titleSearchInputId: 'admin-problem-title-search',
    problemIdSearchInputId: 'admin-problem-id-search',
    create: {
      model: {
        newProblemTitle,
        canCreateProblem: problemActions.canCreateProblem,
        isCreatingProblem: problemActions.isCreatingProblem
      },
      actions: {
        updateNewProblemTitle,
        createProblem: problemActions.handleCreateProblem
      }
    }
  })
  const editor = createProblemEditorViewModel({
    isLoadingDetail: problemDetailResource.isLoadingDetail,
    detailErrorMessage: problemDetailResource.detailErrorMessage,
    selectedProblemDetail: problemDetailResource.selectedProblemDetail,
    busySection,
    canSaveTitle: editorDraft.canSaveTitle,
    canSaveLimits: editorDraft.canSaveLimits,
    canSaveStatement: editorDraft.canSaveStatement,
    canCreateSample: problemActions.canCreateSample,
    canUploadTestcaseZip: problemActions.canUploadTestcaseZip,
    canDeleteLastSample: problemActions.canDeleteLastSample,
    isSavingTitle: problemActions.isSavingTitle,
    isSavingLimits: problemActions.isSavingLimits,
    isSavingStatement: problemActions.isSavingStatement,
    isCreatingSample: problemActions.isCreatingSample,
    isDeletingLastSample: problemActions.isDeletingLastSample,
    isUploadingTestcaseZip: problemActions.isUploadingTestcaseZip,
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
    formatCount,
    isSavingSample: editorDraft.isSavingSample,
    canSaveSample: editorDraft.canSaveSample,
    isLastSample: editorDraft.isLastSample,
    handleSaveTitle: problemActions.handleSaveTitle,
    handleSaveLimits: problemActions.handleSaveLimits,
    handleSaveStatement: problemActions.handleSaveStatement,
    handleCreateSample: problemActions.handleCreateSample,
    handleSaveSample: problemActions.handleSaveSample,
    handleDeleteLastSample: problemActions.handleDeleteLastSample,
    handleTestcaseZipFileChange: editorDraft.handleTestcaseZipFileChange,
    handleUploadTestcaseZip: problemActions.handleUploadTestcaseZip,
    openRejudgeDialog: problemActionFeedback.openRejudgeDialog,
    openDeleteDialog: problemActionFeedback.openDeleteDialog
  })
  const rejudgeDialog = createProblemConfirmDialogViewModel({
    open: problemActionFeedback.rejudgeDialogOpen,
    problemIdInput: problemActionFeedback.rejudgeConfirmProblemId,
    titleInput: problemActionFeedback.rejudgeConfirmTitle,
    selectedProblemDetail: problemDetailResource.selectedProblemDetail,
    isBusy: problemActions.isRejudgingProblem,
    canConfirm: problemActionFeedback.canRejudgeSelectedProblem,
    confirmLabel: rejudgeConfirmLabel,
    titleId: 'admin-problem-rejudge-title',
    kicker: 'confirm rejudge',
    title: '문제 재채점 확인',
    description: '아래 두 값을 모두 정확히 다시 입력해야 재채점을 요청할 수 있습니다.',
    summaryCopy: '현재 문제의 `accepted`, `wrong_answer` 제출을 다시 채점 대기열에 넣습니다.',
    confirmButtonClass: 'admin-problem-rejudge-confirm',
    close: problemActionFeedback.closeRejudgeDialog,
    confirm: problemActions.handleRejudgeProblem
  })
  const deleteDialog = createProblemConfirmDialogViewModel({
    open: problemActionFeedback.deleteDialogOpen,
    problemIdInput: problemActionFeedback.deleteConfirmProblemId,
    titleInput: problemActionFeedback.deleteConfirmTitle,
    selectedProblemDetail: problemDetailResource.selectedProblemDetail,
    isBusy: problemActions.isDeletingProblem,
    canConfirm: problemActionFeedback.canDeleteSelectedProblem,
    confirmLabel: deleteConfirmLabel,
    titleId: 'admin-problem-delete-title',
    kicker: 'confirm delete',
    title: '문제 삭제 확인',
    description: '아래 두 값을 모두 정확히 다시 입력해야 삭제할 수 있습니다.',
    confirmButtonClass: 'admin-problem-delete-confirm',
    close: problemActionFeedback.closeDeleteDialog,
    confirm: problemActions.handleDeleteProblem
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
