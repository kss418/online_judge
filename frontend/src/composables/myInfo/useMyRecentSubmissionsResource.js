import { getSubmissionList } from '@/api/submission'
import { useAsyncResource } from '@/composables/useAsyncResource'
import { formatApiError } from '@/utils/apiError'

export function useMyRecentSubmissionsResource({
  authState
}){
  const recentSubmissionsResource = useAsyncResource({
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

  function resetRecentSubmissions(){
    recentSubmissionsResource.reset({
      preserveHasLoadedOnce: true,
      clearLastArgs: true
    })
  }

  async function loadRecentSubmissions(profileUserId){
    const normalizedProfileUserId = Number(profileUserId)

    if (!Number.isInteger(normalizedProfileUserId) || normalizedProfileUserId <= 0) {
      resetRecentSubmissions()

      return {
        status: 'reset',
        data: recentSubmissionsResource.data.value
      }
    }

    return recentSubmissionsResource.run(normalizedProfileUserId, {
      resetDataOnError: true
    })
  }

  return {
    recentSubmissions: recentSubmissionsResource.data,
    isRecentSubmissionsLoading: recentSubmissionsResource.isLoading,
    recentSubmissionsErrorMessage: recentSubmissionsResource.errorMessage,
    resetRecentSubmissions,
    loadRecentSubmissions
  }
}
