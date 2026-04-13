import { demoteUserToUser, promoteUserToAdmin } from '@/api/userAdminApi'
import { getRoleName } from '@/api/normalizers/permission'
import { noticeStore } from '@/stores/notice/noticeStore'
import { formatApiError } from '@/utils/apiError'
import { ref } from 'vue'

export const permissionLevelToRole = Object.freeze({
  user: 0,
  admin: 1,
  superadmin: 2
})

export function useAdminUserPermissionActions({
  authState,
  canEditPermissions,
  currentUserId,
  refreshCurrentUser,
  patchUser
}){
  const savingUserId = ref(0)

  async function handleRoleAction(user, nextPermissionLevel){
    if (!authState.token || !canEditPermissions.value) {
      if (!canEditPermissions.value) {
        noticeStore.showErrorNotice('권한 변경은 슈퍼어드민만 할 수 있습니다.', {
          duration: 5000
        })
      }

      return
    }

    savingUserId.value = user.user_id

    try {
      if (nextPermissionLevel === permissionLevelToRole.admin) {
        await promoteUserToAdmin(user.user_id, authState.token)
      } else {
        await demoteUserToUser(user.user_id, authState.token)
      }

      patchUser(user.user_id, {
        permission_level: nextPermissionLevel,
        role_name: getRoleName(nextPermissionLevel)
      })

      if (user.user_id === currentUserId.value) {
        await refreshCurrentUser()
      }

      noticeStore.showSuccessNotice(
        nextPermissionLevel === permissionLevelToRole.admin
          ? `${user.user_login_id} 님을 어드민으로 승격했습니다.`
          : `${user.user_login_id} 님을 유저로 강등했습니다.`
      )
    } catch (error) {
      noticeStore.showErrorNotice(
        formatApiError(error, {
          fallback: '권한 변경을 적용하지 못했습니다.'
        }),
        { duration: 5000 }
      )
    } finally {
      savingUserId.value = 0
    }
  }

  async function handlePromoteToAdmin(user){
    return handleRoleAction(user, permissionLevelToRole.admin)
  }

  async function handleDemoteToUser(user){
    return handleRoleAction(user, permissionLevelToRole.user)
  }

  return {
    savingUserId,
    handlePromoteToAdmin,
    handleDemoteToUser
  }
}
