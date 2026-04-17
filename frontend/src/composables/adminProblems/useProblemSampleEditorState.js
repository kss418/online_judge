import { computed, ref } from 'vue'

import { runBusyAction } from '@/composables/adminShared/runBusyAction'
import { useTestcaseZipInput } from '@/composables/adminShared/useTestcaseZipInput'
import { useTestcaseZipUploadAction } from '@/composables/adminShared/useTestcaseZipUploadAction'
import {
  isProblemSampleBusySection,
  makeProblemSampleBusySection,
  problemBusySection
} from '@/composables/adminProblems/problemBusySection'
import { makeSampleDraft } from '@/composables/adminProblems/problemHelpers'
import {
  createProblemSample,
  deleteProblemSample,
  updateProblemSample
} from '@/api/problemSampleApi'
import { formatApiError } from '@/utils/apiError'

export function useProblemSampleEditorState({
  authState,
  busySection,
  formatCount,
  selectedProblemDetail,
  problemDetailResource,
  problemActionFeedback,
  loadSelectedProblem
}){
  const sampleDrafts = ref([])
  const {
    testcaseZipFile,
    testcaseZipInputKey,
    selectedTestcaseZipName,
    resetTestcaseZipSelection,
    handleTestcaseZipFileChange
  } = useTestcaseZipInput({
    onInvalidZip(){
      problemActionFeedback.setActionFeedback({
        message: '',
        error: 'ZIP 파일만 업로드할 수 있습니다.'
      })
    },
    onValidZip(){
      problemActionFeedback.setActionFeedback({
        error: ''
      })
    }
  })
  const isCreatingSample = computed(() => busySection.value === problemBusySection.CREATE_SAMPLE)
  const isDeletingLastSample = computed(() => busySection.value === problemBusySection.DELETE_LAST_SAMPLE)
  const canCreateSample = computed(() =>
    Boolean(selectedProblemDetail.value) &&
    Boolean(authState.token) &&
    !busySection.value
  )
  const canDeleteLastSample = computed(() =>
    Boolean(selectedProblemDetail.value?.samples.length) &&
    Boolean(authState.token) &&
    !busySection.value
  )
  const selectedProblemId = computed(() => Number(selectedProblemDetail.value?.problem_id ?? 0))
  const clearActionFeedback = () => problemActionFeedback.setActionFeedback({
    message: '',
    error: ''
  })

  function reset(options = {}){
    sampleDrafts.value = []

    if (!options.skipTestcaseZipReset) {
      resetTestcaseZipSelection()
    }
  }

  function assignSamples(samples){
    sampleDrafts.value = (Array.isArray(samples) ? samples : []).map(makeSampleDraft)
  }

  function syncSamples(samples){
    assignSamples(samples)
  }

  function getSelectedProblemSample(sampleOrder){
    return selectedProblemDetail.value?.samples.find((sample) => sample.sample_order === sampleOrder) || null
  }

  function getSampleDraft(sampleOrder){
    return sampleDrafts.value.find((sample) => sample.sample_order === sampleOrder) || null
  }

  function isSavingSample(sampleOrder){
    return isProblemSampleBusySection(busySection.value, sampleOrder)
  }

  function canSaveSample(sampleOrder){
    const problemDetail = selectedProblemDetail.value
    if (!problemDetail || !authState.token || busySection.value) {
      return false
    }

    const selectedSample = getSelectedProblemSample(sampleOrder)
    const sampleDraft = getSampleDraft(sampleOrder)
    if (!selectedSample || !sampleDraft) {
      return false
    }

    return (
      sampleDraft.sample_input !== selectedSample.sample_input ||
      sampleDraft.sample_output !== selectedSample.sample_output
    )
  }

  function isLastSample(sampleOrder){
    const samples = selectedProblemDetail.value?.samples || []
    if (!samples.length) {
      return false
    }

    return samples[samples.length - 1].sample_order === sampleOrder
  }

  async function handleCreateSample(){
    if (!authState.token || !selectedProblemDetail.value || !canCreateSample.value) {
      return
    }

    const problemId = selectedProblemDetail.value.problem_id

    return runBusyAction({
      busySection,
      section: problemBusySection.CREATE_SAMPLE,
      clearFeedback: clearActionFeedback,
      run: async () => {
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

        problemDetailResource.setSelectedProblemSamples(nextSamples)
        syncSamples(nextSamples)
        problemDetailResource.applyProblemVersion(problemId, response.version)
        problemActionFeedback.setActionFeedback({
          message: `예제 ${formatCount(nextSampleOrder)}를 추가했습니다.`
        })
      },
      onError: (error) => {
        problemActionFeedback.setActionFeedback({
          error: formatApiError(error, {
            fallback: '공개 예제를 추가하지 못했습니다.'
          })
        })
      }
    })
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

    return runBusyAction({
      busySection,
      section: makeProblemSampleBusySection(sampleOrder),
      clearFeedback: clearActionFeedback,
      run: async () => {
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

        problemDetailResource.setSelectedProblemSamples(nextSamples)
        syncSamples(nextSamples)
        problemDetailResource.applyProblemVersion(problemId, response.version)
        problemActionFeedback.setActionFeedback({
          message: `예제 ${formatCount(sampleOrder)}를 저장했습니다.`
        })
      },
      onError: (error) => {
        problemActionFeedback.setActionFeedback({
          error: formatApiError(error, {
            fallback: '공개 예제를 저장하지 못했습니다.'
          })
        })
      }
    })
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

    return runBusyAction({
      busySection,
      section: problemBusySection.DELETE_LAST_SAMPLE,
      clearFeedback: clearActionFeedback,
      run: async () => {
        const response = await deleteProblemSample(problemId, authState.token)
        const nextSamples = selectedProblemDetail.value.samples.filter(
          (sample) => sample.sample_order !== lastSample.sample_order
        )

        problemDetailResource.setSelectedProblemSamples(nextSamples)
        syncSamples(nextSamples)
        problemDetailResource.applyProblemVersion(problemId, response.version)
        problemActionFeedback.setActionFeedback({
          message: `예제 ${formatCount(lastSample.sample_order)}를 삭제했습니다.`
        })
      },
      onError: (error) => {
        problemActionFeedback.setActionFeedback({
          error: formatApiError(error, {
            fallback: '공개 예제를 삭제하지 못했습니다.'
          })
        })
      }
    })
  }

  const testcaseZipUploadAction = useTestcaseZipUploadAction({
    authState,
    busySection,
    uploadSection: problemBusySection.UPLOAD_TESTCASE_ZIP,
    selectedProblemId,
    testcaseZipFile,
    clearFeedback: clearActionFeedback,
    resetTestcaseZipSelection,
    async afterUpload(response, problemId){
      problemDetailResource.applyProblemVersion(problemId, response.version)
      await loadSelectedProblem(problemId, {
        skipTestcaseZipReset: true
      })
    },
    showSuccess(message){
      problemActionFeedback.setActionFeedback({
        message
      })
    },
    showError(error){
      problemActionFeedback.setActionFeedback({
        error
      })
    },
    formatSuccessMessage(response){
      const testcaseCount = Number(response.testcase_count ?? 0)
      return `비공개 테스트케이스 ${formatCount(testcaseCount)}개를 업로드했습니다.`
    },
    fallbackError: '비공개 테스트케이스 ZIP을 업로드하지 못했습니다.'
  })

  return {
    sampleDrafts,
    testcaseZipFile,
    testcaseZipInputKey,
    selectedTestcaseZipName,
    canCreateSample,
    canDeleteLastSample,
    canUploadTestcaseZip: testcaseZipUploadAction.canUploadTestcaseZip,
    isCreatingSample,
    isDeletingLastSample,
    isUploadingTestcaseZip: testcaseZipUploadAction.isUploadingTestcaseZip,
    reset,
    assignSamples,
    syncSamples,
    getSampleDraft,
    isSavingSample,
    canSaveSample,
    isLastSample,
    resetTestcaseZipSelection,
    handleCreateSample,
    handleSaveSample,
    handleDeleteLastSample,
    handleTestcaseZipFileChange,
    handleUploadTestcaseZip: testcaseZipUploadAction.handleUploadTestcaseZip
  }
}
