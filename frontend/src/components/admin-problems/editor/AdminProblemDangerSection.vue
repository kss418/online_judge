<template>
  <article class="admin-problem-editor-section is-danger">
    <div class="panel-header">
      <div>
        <p class="panel-kicker">danger zone</p>
        <h3>문제 삭제</h3>
      </div>
    </div>

    <p class="admin-problem-danger-copy">
      삭제 전에 문제 번호와 제목을 다시 입력해야 합니다. 제출이 연결된 문제는 삭제가 거부될 수 있습니다.
    </p>

    <div class="admin-problem-section-actions">
      <button
        type="button"
        class="ghost-button admin-problem-danger-button"
        :disabled="Boolean(section.model.busySection)"
        @click="handleOpenDeleteDialog"
      >
        문제 삭제
      </button>
    </div>
  </article>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({
  section: {
    type: Object,
    required: true
  }
})

const section = computed(() => props.section)

function handleOpenDeleteDialog(){
  section.value.actions.openDeleteDialog()
}
</script>

<style scoped>
.admin-problem-editor-section {
  display: grid;
  gap: 1rem;
  padding: 1rem;
  border-radius: 22px;
  border: 1px solid var(--admin-problems-section-border);
  background: var(--admin-problems-section-surface);
  box-shadow: var(--admin-problems-section-shadow);
}

.admin-problem-editor-section.is-danger {
  border-color: rgba(185, 28, 28, 0.16);
  background: linear-gradient(
    180deg,
    rgba(255, 250, 250, 0.98),
    rgba(254, 242, 242, 0.94)
  );
}

.admin-problem-danger-copy {
  margin: 0;
  color: var(--ink-soft);
}

.admin-problem-section-actions {
  display: flex;
  justify-content: flex-end;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

.admin-problem-danger-button {
  min-width: 8.5rem;
  color: var(--danger);
  background: rgba(254, 242, 242, 0.96);
  border-color: rgba(185, 28, 28, 0.18);
}

@media (max-width: 720px) {
  .admin-problem-section-actions {
    justify-content: stretch;
  }
}
</style>
