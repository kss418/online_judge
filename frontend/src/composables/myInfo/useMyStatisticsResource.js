import { getUserSubmissionStatistics } from '@/api/user'
import { useAsyncResource } from '@/composables/useAsyncResource'
import { formatApiError } from '@/utils/apiError'

export function useMyStatisticsResource(){
  const statisticsResource = useAsyncResource({
    initialData: null,
    async load(profileUserId){
      return getUserSubmissionStatistics(profileUserId)
    },
    getErrorMessage(error){
      return formatApiError(error, {
        fallback: '제출 통계를 불러오지 못했습니다.'
      })
    }
  })

  function resetSubmissionStatistics(){
    statisticsResource.reset({
      preserveHasLoadedOnce: true,
      clearLastArgs: true
    })
  }

  async function loadSubmissionStatistics(profileUserId){
    const normalizedProfileUserId = Number(profileUserId)

    if (!Number.isInteger(normalizedProfileUserId) || normalizedProfileUserId <= 0) {
      resetSubmissionStatistics()

      return {
        status: 'reset',
        data: statisticsResource.data.value
      }
    }

    return statisticsResource.run(normalizedProfileUserId, {
      resetDataOnError: true
    })
  }

  return {
    submissionStatistics: statisticsResource.data,
    isStatisticsLoading: statisticsResource.isLoading,
    statisticsErrorMessage: statisticsResource.errorMessage,
    resetSubmissionStatistics,
    loadSubmissionStatistics
  }
}
