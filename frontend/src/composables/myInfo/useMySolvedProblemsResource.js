import { getUserSolvedProblems } from '@/api/userStatisticsApi'
import { useProfileUserIdAsyncResource } from '@/composables/myInfo/profileUserIdResourceShared'
import { formatApiError } from '@/utils/apiError'

export function useMySolvedProblemsResource({
  authState
}){
  const {
    resource: solvedProblemsResource,
    reset: resetSolvedProblems,
    loadByProfileUserId: loadSolvedProblems
  } = useProfileUserIdAsyncResource({
    initialData: [],
    async load(profileUserId){
      const payload = await getUserSolvedProblems(
        profileUserId,
        authState.token || undefined
      )

      return Array.isArray(payload.solved_problems) ? payload.solved_problems : []
    },
    getErrorMessage(error){
      return formatApiError(error, {
        fallback: '푼 문제 목록을 불러오지 못했습니다.'
      })
    }
  })

  return {
    solvedProblems: solvedProblemsResource.data,
    isSolvedProblemsLoading: solvedProblemsResource.isLoading,
    solvedProblemsErrorMessage: solvedProblemsResource.errorMessage,
    resetSolvedProblems,
    loadSolvedProblems
  }
}
