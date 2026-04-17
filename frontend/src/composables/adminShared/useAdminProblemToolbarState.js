import { computed, unref } from 'vue'

import { unwrapRecordValues } from '@/composables/adminShared/adminProblemSelectionHelpers'

export function useAdminProblemToolbarState({
  workspace,
  canManageProblems,
  busySection,
  statusLabel,
  statusTone,
  extraModel
}){
  return computed(() => ({
    model: {
      statusLabel: unref(statusLabel),
      statusTone: unref(statusTone),
      canManageProblems: unref(canManageProblems),
      busySection: unref(busySection),
      isLoadingProblems: workspace.isLoadingProblems.value,
      ...unwrapRecordValues(extraModel)
    },
    actions: {
      refresh: workspace.refreshWorkspace
    }
  }))
}
