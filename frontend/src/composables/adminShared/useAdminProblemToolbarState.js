import { computed, unref } from 'vue'

function unwrapRecordValues(record){
  const nextRecord = {}

  Object.entries(record ?? {}).forEach(([key, value]) => {
    nextRecord[key] = unref(value)
  })

  return nextRecord
}

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
