import { computed, ref } from 'vue'

import {
  createProblem,
  createProblemSample,
  deleteProblem,
  deleteProblemSample,
  rejudgeProblem,
  uploadProblemTestcaseZip,
  updateProblemLimits,
  updateProblemSample,
  updateProblemStatement,
  updateProblemTitle
} from '@/api/problem'
import { parsePositiveInteger } from '@/composables/adminProblems/problemHelpers'

export function useProblemAdminActions({
  authState,
  router,
  formatCount,
  busySection,
  newProblemTitle,
  selectedProblemDetail,
  testcaseZipFile,
  searchMode,
  titleSearchInput,
  problemIdSearchInput,
  titleDraft,
  timeLimitDraft,
  memoryLimitDraft,
  descriptionDraft,
  inputFormatDraft,
  outputFormatDraft,
  noteDraft,
  canSaveTitle,
  canSaveLimits,
  canSaveStatement,
  canSaveSample,
  getSampleDraft,
  makeSampleBusyKey,
  resetEditorDrafts,
  syncSampleDrafts,
  setSelectedProblemSamples,
  applySelectedProblemVersion,
  mergeProblemSummary,
  loadProblems,
  loadSelectedProblem
}){
  const actionMessage = ref('')
  const actionErrorMessage = ref('')
  const rejudgeDialogOpen = ref(false)
  const rejudgeConfirmProblemId = ref('')
  const rejudgeConfirmTitle = ref('')
  const deleteDialogOpen = ref(false)
  const deleteConfirmProblemId = ref('')
  const deleteConfirmTitle = ref('')

  const canCreateProblem = computed(() =>
    Boolean(authState.token) &&
    !busySection.value &&
    Boolean(newProblemTitle.value.trim())
  )
  const isCreatingProblem = computed(() => busySection.value === 'create')
  const isSavingTitle = computed(() => busySection.value === 'title')
  const isSavingLimits = computed(() => busySection.value === 'limits')
  const isSavingStatement = computed(() => busySection.value === 'statement')
  const isCreatingSample = computed(() => busySection.value === 'sample:create')
  const isDeletingLastSample = computed(() => busySection.value === 'sample:delete-last')
  const isUploadingTestcaseZip = computed(() => busySection.value === 'testcase:upload')
  const isRejudgingProblem = computed(() => busySection.value === 'rejudge')
  const isDeletingProblem = computed(() => busySection.value === 'delete')
  const canCreateSample = computed(() =>
    Boolean(selectedProblemDetail.value) &&
    Boolean(authState.token) &&
    !busySection.value
  )
  const canUploadTestcaseZip = computed(() =>
    Boolean(selectedProblemDetail.value) &&
    Boolean(authState.token) &&
    Boolean(testcaseZipFile.value) &&
    !busySection.value
  )
  const canDeleteLastSample = computed(() =>
    Boolean(selectedProblemDetail.value?.samples.length) &&
    Boolean(authState.token) &&
    !busySection.value
  )
  const canDeleteSelectedProblem = computed(() => {
    if (!selectedProblemDetail.value || !deleteDialogOpen.value || busySection.value) {
      return false
    }

    const expectedProblemId = String(selectedProblemDetail.value.problem_id)
    return (
      deleteConfirmProblemId.value.trim() === expectedProblemId &&
      deleteConfirmTitle.value === selectedProblemDetail.value.title
    )
  })
  const canRejudgeSelectedProblem = computed(() => {
    if (!selectedProblemDetail.value || !rejudgeDialogOpen.value || busySection.value) {
      return false
    }

    const expectedProblemId = String(selectedProblemDetail.value.problem_id)
    return (
      rejudgeConfirmProblemId.value.trim() === expectedProblemId &&
      rejudgeConfirmTitle.value === selectedProblemDetail.value.title
    )
  })

  function clearActionError(){
    actionErrorMessage.value = ''
  }

  function setActionFeedback({ message, error }){
    if (typeof message !== 'undefined') {
      actionMessage.value = message
    }

    if (typeof error !== 'undefined') {
      actionErrorMessage.value = error
    }
  }

  async function handleCreateProblem(){
    if (!authState.token || !canCreateProblem.value) {
      return
    }

    busySection.value = 'create'
    actionErrorMessage.value = ''
    actionMessage.value = ''

    try {
      const response = await createProblem({
        title: newProblemTitle.value.trim()
      }, authState.token)

      newProblemTitle.value = ''
      searchMode.value = 'title'
      titleSearchInput.value = ''
      problemIdSearchInput.value = ''
      actionMessage.value = `문제 #${formatCount(response.problem_id)}를 생성했습니다.`

      await router.replace({
        query: {
          problemId: String(response.problem_id ?? '')
        }
      })
      await loadProblems({
        preferredProblemId: Number(response.problem_id ?? 0)
      })
    } catch (error) {
      actionErrorMessage.value = error instanceof Error
        ? error.message
        : '문제를 생성하지 못했습니다.'
    } finally {
      busySection.value = ''
    }
  }

  async function handleSaveTitle(){
    if (!authState.token || !selectedProblemDetail.value || !canSaveTitle.value) {
      return
    }

    busySection.value = 'title'
    actionErrorMessage.value = ''
    actionMessage.value = ''

    try {
      const nextTitle = titleDraft.value.trim()
      const response = await updateProblemTitle(selectedProblemDetail.value.problem_id, {
        title: nextTitle
      }, authState.token)

      selectedProblemDetail.value = {
        ...selectedProblemDetail.value,
        title: nextTitle
      }
      titleDraft.value = nextTitle
      applySelectedProblemVersion(selectedProblemDetail.value.problem_id, response.version)
      mergeProblemSummary(selectedProblemDetail.value.problem_id, {
        title: nextTitle
      })
      actionMessage.value = `문제 #${formatCount(selectedProblemDetail.value.problem_id)} 제목을 저장했습니다.`
    } catch (error) {
      actionErrorMessage.value = error instanceof Error
        ? error.message
        : '문제 제목을 저장하지 못했습니다.'
    } finally {
      busySection.value = ''
    }
  }

  async function handleSaveLimits(){
    if (!authState.token || !selectedProblemDetail.value || !canSaveLimits.value) {
      return
    }

    const nextTimeLimit = parsePositiveInteger(timeLimitDraft.value)
    const nextMemoryLimit = parsePositiveInteger(memoryLimitDraft.value)
    if (nextTimeLimit == null || nextMemoryLimit == null) {
      return
    }

    busySection.value = 'limits'
    actionErrorMessage.value = ''
    actionMessage.value = ''

    try {
      const response = await updateProblemLimits(selectedProblemDetail.value.problem_id, {
        time_limit_ms: nextTimeLimit,
        memory_limit_mb: nextMemoryLimit
      }, authState.token)

      selectedProblemDetail.value = {
        ...selectedProblemDetail.value,
        limits: {
          time_limit_ms: nextTimeLimit,
          memory_limit_mb: nextMemoryLimit
        }
      }
      timeLimitDraft.value = String(nextTimeLimit)
      memoryLimitDraft.value = String(nextMemoryLimit)
      applySelectedProblemVersion(selectedProblemDetail.value.problem_id, response.version)
      mergeProblemSummary(selectedProblemDetail.value.problem_id, {
        time_limit_ms: nextTimeLimit,
        memory_limit_mb: nextMemoryLimit
      })
      actionMessage.value = `문제 #${formatCount(selectedProblemDetail.value.problem_id)} 제한을 저장했습니다.`
    } catch (error) {
      actionErrorMessage.value = error instanceof Error
        ? error.message
        : '문제 제한을 저장하지 못했습니다.'
    } finally {
      busySection.value = ''
    }
  }

  async function handleSaveStatement(){
    if (!authState.token || !selectedProblemDetail.value || !canSaveStatement.value) {
      return
    }

    busySection.value = 'statement'
    actionErrorMessage.value = ''
    actionMessage.value = ''

    try {
      const nextStatement = {
        description: descriptionDraft.value,
        input_format: inputFormatDraft.value,
        output_format: outputFormatDraft.value,
        note: noteDraft.value === '' ? null : noteDraft.value
      }

      const response = await updateProblemStatement(
        selectedProblemDetail.value.problem_id,
        nextStatement,
        authState.token
      )

      selectedProblemDetail.value = {
        ...selectedProblemDetail.value,
        statement: {
          description: descriptionDraft.value,
          input_format: inputFormatDraft.value,
          output_format: outputFormatDraft.value,
          note: noteDraft.value
        }
      }
      applySelectedProblemVersion(selectedProblemDetail.value.problem_id, response.version)
      actionMessage.value = `문제 #${formatCount(selectedProblemDetail.value.problem_id)} 설명을 저장했습니다.`
    } catch (error) {
      actionErrorMessage.value = error instanceof Error
        ? error.message
        : '문제 설명을 저장하지 못했습니다.'
    } finally {
      busySection.value = ''
    }
  }

  async function handleCreateSample(){
    if (!authState.token || !selectedProblemDetail.value || !canCreateSample.value) {
      return
    }

    const problemId = selectedProblemDetail.value.problem_id
    busySection.value = 'sample:create'
    actionErrorMessage.value = ''
    actionMessage.value = ''

    try {
      const response = await createProblemSample(problemId, authState.token)

      const nextSampleOrder = Number(response.sample_order ?? selectedProblemDetail.value.samples.length + 1)
      const nextSamples = [
        ...selectedProblemDetail.value.samples,
        {
          sample_order: nextSampleOrder,
          sample_input: '',
          sample_output: ''
        }
      ]
      setSelectedProblemSamples(nextSamples)
      syncSampleDrafts(nextSamples)
      applySelectedProblemVersion(problemId, response.version)
      actionMessage.value = `예제 ${formatCount(nextSampleOrder)}를 추가했습니다.`
    } catch (error) {
      actionErrorMessage.value = error instanceof Error
        ? error.message
        : '공개 예제를 추가하지 못했습니다.'
    } finally {
      busySection.value = ''
    }
  }

  async function handleSaveSample(sampleOrder){
    if (!authState.token || !selectedProblemDetail.value || !canSaveSample(sampleOrder)) {
      return
    }

    const problemId = selectedProblemDetail.value.problem_id
    const sampleDraft = getSampleDraft(sampleOrder)
    if (!sampleDraft) {
      return
    }

    busySection.value = makeSampleBusyKey(sampleOrder)
    actionErrorMessage.value = ''
    actionMessage.value = ''

    try {
      const response = await updateProblemSample(problemId, sampleOrder, {
        sample_input: sampleDraft.sample_input,
        sample_output: sampleDraft.sample_output
      }, authState.token)

      const nextSamples = selectedProblemDetail.value.samples.map((sample) =>
        sample.sample_order === sampleOrder
          ? {
            ...sample,
            sample_input: response.sample_input ?? sampleDraft.sample_input,
            sample_output: response.sample_output ?? sampleDraft.sample_output
          }
          : sample
      )
      setSelectedProblemSamples(nextSamples)
      syncSampleDrafts(nextSamples)
      applySelectedProblemVersion(problemId, response.version)
      actionMessage.value = `예제 ${formatCount(sampleOrder)}를 저장했습니다.`
    } catch (error) {
      actionErrorMessage.value = error instanceof Error
        ? error.message
        : '공개 예제를 저장하지 못했습니다.'
    } finally {
      busySection.value = ''
    }
  }

  async function handleDeleteLastSample(){
    if (!authState.token || !selectedProblemDetail.value || !canDeleteLastSample.value) {
      return
    }

    const problemId = selectedProblemDetail.value.problem_id
    const lastSample = selectedProblemDetail.value.samples[selectedProblemDetail.value.samples.length - 1]
    if (!lastSample) {
      return
    }

    busySection.value = 'sample:delete-last'
    actionErrorMessage.value = ''
    actionMessage.value = ''

    try {
      const response = await deleteProblemSample(problemId, authState.token)
      const nextSamples = selectedProblemDetail.value.samples.filter(
        (sample) => sample.sample_order !== lastSample.sample_order
      )
      setSelectedProblemSamples(nextSamples)
      syncSampleDrafts(nextSamples)
      applySelectedProblemVersion(problemId, response.version)
      actionMessage.value = `예제 ${formatCount(lastSample.sample_order)}를 삭제했습니다.`
    } catch (error) {
      actionErrorMessage.value = error instanceof Error
        ? error.message
        : '공개 예제를 삭제하지 못했습니다.'
    } finally {
      busySection.value = ''
    }
  }

  async function handleUploadTestcaseZip(){
    if (!authState.token || !selectedProblemDetail.value || !canUploadTestcaseZip.value || !testcaseZipFile.value) {
      return
    }

    const problemId = selectedProblemDetail.value.problem_id
    const uploadFile = testcaseZipFile.value
    busySection.value = 'testcase:upload'
    actionErrorMessage.value = ''
    actionMessage.value = ''

    try {
      const response = await uploadProblemTestcaseZip(problemId, uploadFile, authState.token)
      applySelectedProblemVersion(problemId, response.version)
      await loadSelectedProblem(problemId)

      const testcaseCount = Number(response.testcase_count ?? 0)
      actionMessage.value = `비공개 테스트케이스 ${formatCount(testcaseCount)}개를 업로드했습니다.`
    } catch (error) {
      actionErrorMessage.value = error instanceof Error
        ? error.message
        : '비공개 테스트케이스 ZIP을 업로드하지 못했습니다.'
    } finally {
      busySection.value = ''
    }
  }

  function openDeleteDialog(){
    if (!selectedProblemDetail.value || busySection.value) {
      return
    }

    deleteConfirmProblemId.value = ''
    deleteConfirmTitle.value = ''
    deleteDialogOpen.value = true
  }

  function openRejudgeDialog(){
    if (!selectedProblemDetail.value || busySection.value) {
      return
    }

    rejudgeConfirmProblemId.value = ''
    rejudgeConfirmTitle.value = ''
    rejudgeDialogOpen.value = true
  }

  function closeDeleteDialog(force = false){
    if (!force && isDeletingProblem.value) {
      return
    }

    deleteDialogOpen.value = false
    deleteConfirmProblemId.value = ''
    deleteConfirmTitle.value = ''
  }

  function closeRejudgeDialog(force = false){
    if (!force && isRejudgingProblem.value) {
      return
    }

    rejudgeDialogOpen.value = false
    rejudgeConfirmProblemId.value = ''
    rejudgeConfirmTitle.value = ''
  }

  async function handleRejudgeProblem(){
    if (!authState.token || !selectedProblemDetail.value || !canRejudgeSelectedProblem.value) {
      return
    }

    const rejudgingProblemId = selectedProblemDetail.value.problem_id
    busySection.value = 'rejudge'
    actionErrorMessage.value = ''
    actionMessage.value = ''

    try {
      await rejudgeProblem(rejudgingProblemId, authState.token)
      closeRejudgeDialog(true)
      actionMessage.value = `문제 #${formatCount(rejudgingProblemId)} 재채점을 요청했습니다.`
    } catch (error) {
      actionErrorMessage.value = error instanceof Error
        ? error.message
        : '문제 재채점을 요청하지 못했습니다.'
    } finally {
      busySection.value = ''
    }
  }

  async function handleDeleteProblem(){
    if (!authState.token || !selectedProblemDetail.value || !canDeleteSelectedProblem.value) {
      return
    }

    const deletingProblemId = selectedProblemDetail.value.problem_id
    busySection.value = 'delete'
    actionErrorMessage.value = ''
    actionMessage.value = ''

    try {
      await deleteProblem(deletingProblemId, authState.token)
      closeDeleteDialog(true)
      actionMessage.value = `문제 #${formatCount(deletingProblemId)}를 삭제했습니다.`
      await loadProblems()
    } catch (error) {
      actionErrorMessage.value = error instanceof Error
        ? error.message
        : '문제를 삭제하지 못했습니다.'
    } finally {
      busySection.value = ''
    }
  }

  return {
    actionMessage,
    actionErrorMessage,
    rejudgeDialogOpen,
    rejudgeConfirmProblemId,
    rejudgeConfirmTitle,
    deleteDialogOpen,
    deleteConfirmProblemId,
    deleteConfirmTitle,
    canCreateProblem,
    isCreatingProblem,
    isSavingTitle,
    isSavingLimits,
    isSavingStatement,
    isCreatingSample,
    isDeletingLastSample,
    isUploadingTestcaseZip,
    isRejudgingProblem,
    isDeletingProblem,
    canCreateSample,
    canUploadTestcaseZip,
    canDeleteLastSample,
    canDeleteSelectedProblem,
    canRejudgeSelectedProblem,
    clearActionError,
    setActionFeedback,
    handleCreateProblem,
    handleSaveTitle,
    handleSaveLimits,
    handleSaveStatement,
    handleCreateSample,
    handleSaveSample,
    handleDeleteLastSample,
    handleUploadTestcaseZip,
    openDeleteDialog,
    openRejudgeDialog,
    closeDeleteDialog,
    closeRejudgeDialog,
    handleRejudgeProblem,
    handleDeleteProblem
  }
}
