import { getUserList } from '@/api/userAdminApi'
import { useAsyncResource } from '@/composables/useAsyncResource'
import {
  createUserListErrorMessage,
  patchUserInList
} from '@/composables/users/userListResourceShared'

function createInitialUserList(){
  return []
}

export function useAdminUserListResource({
  authState,
  canLoad
}){
  const userListResource = useAsyncResource({
    initialData: createInitialUserList,
    async load(token){
      const response = await getUserList(token)
      return Array.isArray(response.users) ? response.users : []
    },
    getErrorMessage: createUserListErrorMessage
  })

  function resetUsers(){
    userListResource.reset({
      preserveHasLoadedOnce: true,
      clearLastArgs: true
    })
  }

  async function loadUsers(){
    if (!authState?.token || !canLoad?.value) {
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
    userListResource.mutate((users) => patchUserInList(users, userId, patch))
  }

  return {
    users: userListResource.data,
    isLoading: userListResource.isLoading,
    errorMessage: userListResource.errorMessage,
    resetUsers,
    loadUsers,
    patchUser
  }
}
