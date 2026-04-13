import { getUserList } from '@/api/userAdminApi'
import { useAsyncResource } from '@/composables/useAsyncResource'
import { formatApiError } from '@/utils/apiError'

export function useAdminUsersListResource({
  authState,
  canManageUsers
}){
  const userListResource = useAsyncResource({
    initialData: [],
    async load(token){
      const response = await getUserList(token)
      return Array.isArray(response.users) ? response.users : []
    },
    getErrorMessage(error){
      return formatApiError(error, {
        fallback: '유저 목록을 불러오지 못했습니다.'
      })
    }
  })

  function resetUsers(){
    userListResource.reset({
      preserveHasLoadedOnce: true,
      clearLastArgs: true
    })
  }

  async function loadUsers(){
    if (!authState.token || !canManageUsers.value) {
      resetUsers()

      return {
        status: 'reset',
        data: userListResource.data.value
      }
    }

    return userListResource.run(authState.token, {
      resetDataOnError: true
    })
  }

  function patchUser(userId, patch){
    userListResource.mutate((users) =>
      users.map((user) =>
        user.user_id === userId
          ? {
            ...user,
            ...patch
          }
          : user
      )
    )
  }

  return {
    isLoading: userListResource.isLoading,
    errorMessage: userListResource.errorMessage,
    users: userListResource.data,
    resetUsers,
    loadUsers,
    patchUser
  }
}
