import { useAdminProblemSidebarModel } from '@/composables/adminShared/useAdminProblemSidebarModel'
import { useAdminProblemToolbarState } from '@/composables/adminShared/useAdminProblemToolbarState'

export function useAdminProblemSelectionPageShell({
  workspace,
  canManageProblems,
  busySection,
  statusLabel,
  statusTone,
  toolbarExtraModel,
  formatCount,
  titleSearchInputId,
  problemIdSearchInputId,
  sidebarCreate
}){
  const toolbar = useAdminProblemToolbarState({
    workspace,
    canManageProblems,
    busySection,
    statusLabel,
    statusTone,
    extraModel: toolbarExtraModel
  })
  const sidebar = useAdminProblemSidebarModel({
    workspace,
    busySection,
    formatCount,
    titleSearchInputId,
    problemIdSearchInputId,
    create: sidebarCreate
  })

  return {
    shell: workspace.shell,
    toolbar,
    sidebar
  }
}
