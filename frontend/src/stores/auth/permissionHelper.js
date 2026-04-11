import {
  getRoleName,
  normalizePermissionLevel
} from '@/api/normalizers/permission'

export function normalizeCurrentUser(user){
  if (!user) {
    return null
  }

  const permissionLevel = normalizePermissionLevel(user.permission_level)
  const normalizedUserLoginId =
    typeof user.user_login_id === 'string' ? user.user_login_id : ''

  return {
    id: Number(user.id ?? user.user_id ?? 0),
    user_login_id: normalizedUserLoginId,
    permission_level: permissionLevel,
    role_name: user.role_name || getRoleName(permissionLevel)
  }
}

export function getCurrentPermissionLevel(user){
  return normalizePermissionLevel(user?.permission_level)
}

export function hasPermissionLevel(user, requiredLevel){
  return getCurrentPermissionLevel(user) >= Number(requiredLevel ?? 0)
}

export function isAdmin(user){
  return hasPermissionLevel(user, 1)
}

export function isSuperAdmin(user){
  return hasPermissionLevel(user, 2)
}

export function canManageProblems(user){
  return hasPermissionLevel(user, 1)
}

export function canManageUsers(user){
  return hasPermissionLevel(user, 2)
}

export function canRejudgeSubmissions(user){
  return hasPermissionLevel(user, 1)
}
