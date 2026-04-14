import { computed } from 'vue'

import { runBusyAction } from '@/composables/adminShared/runBusyAction'
import {
  makeProblemSampleBusySection,
  problemBusySection
} from '@/composables/adminProblems/problemBusySection'
import {
  createProblemSample,
  deleteProblemSample,
  updateProblemSample
} from '@/api/problemSampleApi'
import { formatApiError } from '@/utils/apiError'

export function useProblemSampleActions({
  authState,
  busySection,
  formatCount,
  selectedProblemDetail,
  canSaveSample,
  getSampleDraft,
  syncSampleDrafts,
  setSelectedProblemSamples,
  applySelectedProblemVersion,
  setActionFeedback
}){
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
  const clearActionFeedback = () => setActionFeedback({
    message: '',
    error: ''
  })

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
        setSelectedProblemSamples(nextSamples)
        syncSampleDrafts(nextSamples)
        applySelectedProblemVersion(problemId, response.version)
        setActionFeedback({
          message: `예제 ${formatCount(nextSampleOrder)}를 추가했습니다.`
        })
      },
      onError: (error) => {
        setActionFeedback({
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
        setSelectedProblemSamples(nextSamples)
        syncSampleDrafts(nextSamples)
        applySelectedProblemVersion(problemId, response.version)
        setActionFeedback({
          message: `예제 ${formatCount(sampleOrder)}를 저장했습니다.`
        })
      },
      onError: (error) => {
        setActionFeedback({
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
        setSelectedProblemSamples(nextSamples)
        syncSampleDrafts(nextSamples)
        applySelectedProblemVersion(problemId, response.version)
        setActionFeedback({
          message: `예제 ${formatCount(lastSample.sample_order)}를 삭제했습니다.`
        })
      },
      onError: (error) => {
        setActionFeedback({
          error: formatApiError(error, {
            fallback: '공개 예제를 삭제하지 못했습니다.'
          })
        })
      }
    })
  }

  return {
    isCreatingSample,
    isDeletingLastSample,
    canCreateSample,
    canDeleteLastSample,
    handleCreateSample,
    handleSaveSample,
    handleDeleteLastSample
  }
}
