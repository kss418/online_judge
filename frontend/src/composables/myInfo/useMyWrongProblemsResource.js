import { getUserWrongProblems } from '@/api/userStatisticsApi'
import { useProfileUserIdAsyncResource } from '@/composables/myInfo/profileUserIdResourceShared'
import { formatApiError } from '@/utils/apiError'

export function useMyWrongProblemsResource({
  authState
}){
  const {
    resource: wrongProblemsResource,
    reset: resetWrongProblems,
    loadByProfileUserId: loadWrongProblems
  } = useProfileUserIdAsyncResource({
    initialData: [],
    async load(profileUserId){
      const payload = await getUserWrongProblems(
        profileUserId,
        authState.token || undefined
      )

      return Array.isArray(payload.wrong_problems) ? payload.wrong_problems : []
    },
    getErrorMessage(error){
      return formatApiError(error, {
        fallback: '틀린 문제 목록을 불러오지 못했습니다.'
      })
    }
  })

  return {
    wrongProblems: wrongProblemsResource.data,
    isWrongProblemsLoading: wrongProblemsResource.isLoading,
    wrongProblemsErrorMessage: wrongProblemsResource.errorMessage,
    resetWrongProblems,
    loadWrongProblems
  }
}
