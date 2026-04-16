import { getUserSubmissionStatistics } from '@/api/userStatisticsApi'
import { useProfileUserIdAsyncResource } from '@/composables/myInfo/profileUserIdResourceShared'
import { formatApiError } from '@/utils/apiError'

export function useMyStatisticsResource(){
  const {
    resource: statisticsResource,
    reset: resetSubmissionStatistics,
    loadByProfileUserId: loadSubmissionStatistics
  } = useProfileUserIdAsyncResource({
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

  return {
    submissionStatistics: statisticsResource.data,
    isStatisticsLoading: statisticsResource.isLoading,
    statisticsErrorMessage: statisticsResource.errorMessage,
    resetSubmissionStatistics,
    loadSubmissionStatistics
  }
}
