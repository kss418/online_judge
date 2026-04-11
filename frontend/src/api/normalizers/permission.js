import {
  normalizeInteger,
  normalizePositiveInteger
} from '@/api/normalizers/common'

export function normalizePermissionLevel(value){
  const numericValue = normalizeInteger(value, 0)

  if (numericValue >= 0 && numericValue <= 2) {
    return numericValue
  }

  if (numericValue >= 100) {
    return 2
  }

  if (numericValue >= 10) {
    return 1
  }

  return 0
}

export function getRoleName(permissionLevel){
  const normalizedPermissionLevel = normalizePositiveInteger(permissionLevel, 0)

  if (normalizedPermissionLevel >= 2) {
    return 'superadmin'
  }

  if (normalizedPermissionLevel >= 1) {
    return 'admin'
  }

  return 'user'
}
