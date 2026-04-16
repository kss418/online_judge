<template>
  <section class="admin-testcases-editor-panel">
    <div
      v-if="editor.shell.mode !== 'ready'"
      class="empty-state"
      :class="{ 'error-state': editor.shell.mode === 'error' }"
    >
      <p>{{ editor.shell.message }}</p>
    </div>

    <template v-else-if="editor.shell.mode === 'ready'">
      <AdminTestcasesEditorHeader :section="editor.sections.header" />

      <AdminTestcaseUploadControls :section="editor.sections.upload" />

      <AdminTestcaseCreateSection :section="editor.sections.create" />

      <AdminTestcaseReorderPanel :section="editor.sections.reorder">
        <AdminTestcaseDetailCard :section="editor.sections.detail" />
      </AdminTestcaseReorderPanel>
    </template>
  </section>
</template>

<script setup>
import { computed } from 'vue'

import AdminTestcaseCreateSection from '@/components/admin-testcases/editor/AdminTestcaseCreateSection.vue'
import AdminTestcaseDetailCard from '@/components/admin-testcases/editor/AdminTestcaseDetailCard.vue'
import AdminTestcaseReorderPanel from '@/components/admin-testcases/editor/AdminTestcaseReorderPanel.vue'
import AdminTestcasesEditorHeader from '@/components/admin-testcases/editor/AdminTestcasesEditorHeader.vue'
import AdminTestcaseUploadControls from '@/components/admin-testcases/editor/AdminTestcaseUploadControls.vue'

const props = defineProps({
  editor: {
    type: Object,
    required: true
  }
})

const editor = computed(() => props.editor)
</script>

<style scoped>
.admin-testcases-editor-panel {
  display: grid;
  gap: 1rem;
  min-height: 40rem;
  padding: 1rem;
  border: 1px solid var(--admin-testcases-shell-border);
  border-radius: 24px;
  background: var(--admin-testcases-shell-surface);
  box-shadow: var(--admin-testcases-shell-shadow);
}
</style>
