import { computed, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'

import { useProblemActionFeedback } from '@/composables/adminProblems/useProblemActionFeedback'
import { useProblemAdminActions } from '@/composables/adminProblems/useProblemAdminActions'
import { useProblemDetailResource } from '@/composables/adminProblems/useProblemDetailResource'
import { useProblemEditorDraft } from '@/composables/adminProblems/useProblemEditorDraft'
import { formatProblemLimit } from '@/composables/adminProblems/problemHelpers'
import { useAdminProblemCatalogQuery } from '@/composables/adminShared/useAdminProblemCatalogQuery'
import { useAdminProblemCatalogResource } from '@/composables/adminShared/useAdminProblemCatalogResource'
import { useAdminProblemRouteCatalogReload } from '@/composables/adminShared/useAdminProblemRouteCatalogReload'
import { useProtectedAdminPageAccess } from '@/composables/adminShared/useProtectedAdminPageAccess'
import { authStore } from '@/stores/auth/authStore'
import { noticeStore } from '@/stores/notice/noticeStore'
import { formatCount } from '@/utils/numberFormat'

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

  async function loadSelectedProblem(problemId, options = {}){
    const normalizedProblemId = Number(problemId)
    if (!Number.isInteger(normalizedProblemId) || normalizedProblemId <= 0) {
      return
    }

    selectedProblemId.value = normalizedProblemId
    problemActionFeedback.clearActionError()
    editorDraft.resetEditorDrafts({
      skipTestcaseZipReset: options.skipTestcaseZipReset === true
    })

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
    await problemQuery.selectCreatedProblem(problemId)
  }

  function resetPageState(){
    problemQuery.resetSearchControls()
    newProblemTitle.value = ''
    busySection.value = ''
    problemListResource.resetProblems()
    clearSelectedProblemState()
    problemActionFeedback.resetActionState()
  }

  const pageAccess = useProtectedAdminPageAccess({
    authState,
    initializeAuth,
    isAuthenticated,
    hasAccess: canManageProblems,
    onDenied: resetPageState,
    async onAllowed(){
      await problemQuery.syncFromRouteAndReload()
    },
    loggedOutMessage: '문제 관리 페이지는 로그인한 관리자만 사용할 수 있습니다.',
    deniedMessage: '이 페이지는 관리자만 접근할 수 있습니다.'
  })

  useAdminProblemRouteCatalogReload({
    pageAccess,
    query: problemQuery
  })

  async function refreshProblems(){
    await loadProblems({
      preferredProblemId: problemQuery.preferredProblemIdForReload.value
    })
  }

  function updateNewProblemTitle(value){
    newProblemTitle.value = value
  }

  const isLoadingProblems = computed(() =>
    pageAccess.accessState.value === 'initializing' || problemListResource.isLoadingProblems.value
  )
  const toolbarStatusLabel = computed(() =>
    isLoadingProblems.value ? 'Loading' : `${problemListResource.problemCount.value} Problems`
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

  const shell = computed(() => ({
    state: pageAccess.accessState.value,
    message: pageAccess.accessMessage.value
  }))
  const toolbar = computed(() => ({
    model: {
      statusLabel: toolbarStatusLabel.value,
      statusTone: toolbarStatusTone.value,
      canManageProblems: canManageProblems.value,
      isLoadingProblems: isLoadingProblems.value,
      busySection: busySection.value
    },
    actions: {
      refresh: refreshProblems
    }
  }))
  const sidebar = computed(() => ({
    model: {
      searchMode: problemQuery.searchMode.value,
      titleSearchInput: problemQuery.titleSearchInput.value,
      problemIdSearchInput: problemQuery.problemIdSearchInput.value,
      titleSearchInputId: 'admin-problem-title-search',
      problemIdSearchInputId: 'admin-problem-id-search',
      isLoadingProblems: isLoadingProblems.value,
      busySection: busySection.value,
      hasAppliedSearch: problemQuery.hasAppliedSearch.value,
      problemListCaption: problemQuery.problemListCaption.value,
      problemCount: problemListResource.problemCount.value,
      listErrorMessage: problemListResource.listErrorMessage.value,
      emptyProblemListMessage: problemQuery.emptyProblemListMessage.value,
      problems: problemListResource.problems.value,
      selectedProblemId: selectedProblemId.value,
      formatCount,
      formatProblemLimit
    },
    actions: {
      updateTitleSearchInput: problemQuery.updateTitleSearchInput,
      updateProblemIdSearchInput: problemQuery.updateProblemIdSearchInput,
      setSearchMode: problemQuery.setSearchMode,
      submitSearch: problemQuery.submitSearch,
      resetSearch: problemQuery.resetSearch,
      selectProblem
    },
    create: {
      model: {
        newProblemTitle: newProblemTitle.value,
        canCreateProblem: problemActions.canCreateProblem.value,
        isCreatingProblem: problemActions.isCreatingProblem.value
      },
      actions: {
        updateNewProblemTitle,
        createProblem: problemActions.handleCreateProblem
      }
    }
  }))
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
