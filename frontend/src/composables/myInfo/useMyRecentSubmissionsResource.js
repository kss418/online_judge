import { getSubmissionList } from '@/api/submission'
import { useProfileUserIdAsyncResource } from '@/composables/myInfo/profileUserIdResourceShared'
import { formatApiError } from '@/utils/apiError'

export function useMyRecentSubmissionsResource({
  authState
}){
  const {
    resource: recentSubmissionsResource,
    reset: resetRecentSubmissions,
    loadByProfileUserId: loadRecentSubmissions
  } = useProfileUserIdAsyncResource({
    initialData: [],
    async load(profileUserId){
      const payload = await getSubmissionList({
        userId: profileUserId,
        limit: 10,
        bearerToken: authState.token || undefined
      })

      return Array.isArray(payload.submissions) ? payload.submissions : []
    },
    getErrorMessage(error){
      return formatApiError(error, {
        fallback: '최근 제출 목록을 불러오지 못했습니다.'
      })
    }
  })

  return {
    recentSubmissions: recentSubmissionsResource.data,
    isRecentSubmissionsLoading: recentSubmissionsResource.isLoading,
    recentSubmissionsErrorMessage: recentSubmissionsResource.errorMessage,
    resetRecentSubmissions,
    loadRecentSubmissions
  }
}
