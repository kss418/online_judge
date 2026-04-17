<template>
  <AdminSplitWorkspaceShell
    :access-state="shell.state"
    :access-message="shell.message"
  >
    <template #toolbar>
      <AdminProblemsToolbar :toolbar="toolbar" />
    </template>

    <template #sidebar>
      <AdminProblemSelectionSidebar :sidebar="sidebar" />
    </template>

    <AdminProblemsEditor :editor="editor" />
  </AdminSplitWorkspaceShell>

  <AdminProblemConfirmDialog :dialog="dialogs.rejudge" />

  <AdminProblemConfirmDialog :dialog="dialogs.delete" />
</template>

<script setup>
import AdminProblemSelectionSidebar from '@/components/adminShared/AdminProblemSelectionSidebar.vue'
import AdminSplitWorkspaceShell from '@/components/adminShared/AdminSplitWorkspaceShell.vue'
import AdminProblemConfirmDialog from '@/components/admin-problems/AdminProblemConfirmDialog.vue'
import AdminProblemsEditor from '@/components/admin-problems/AdminProblemsEditor.vue'
import AdminProblemsToolbar from '@/components/admin-problems/AdminProblemsToolbar.vue'
import { useAdminProblemsPage } from '@/composables/useAdminProblemsPage'

const {
  shell,
  toolbar,
  sidebar,
  editor,
  dialogs
} = useAdminProblemsPage()
</script>
