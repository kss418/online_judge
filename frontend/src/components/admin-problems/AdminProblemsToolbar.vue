<template>
  <div class="admin-problems-toolbar">
    <div>
      <p class="panel-kicker">Problem Admin</p>
      <h3>문제 관리</h3>
      <p class="admin-problems-copy">
        문제를 생성하고 제목, 제한, 설명, 삭제를 한 화면에서 관리할 수 있습니다.
      </p>
    </div>

    <div class="admin-problems-actions">
      <StatusBadge
        :label="toolbar.model.statusLabel"
        :tone="toolbar.model.statusTone"
      />
      <button
        v-if="toolbar.model.canManageProblems"
        type="button"
        class="ghost-button"
        :disabled="toolbar.model.isLoadingProblems || Boolean(toolbar.model.busySection)"
        @click="toolbar.actions.refresh()"
      >
        새로고침
      </button>
    </div>
  </div>
</template>

<script setup>
import StatusBadge from '@/components/StatusBadge.vue'

defineProps({
  toolbar: {
    type: Object,
    required: true
  }
})
</script>

<style scoped>
.admin-problems-toolbar {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: start;
}

.admin-problems-copy {
  margin: 0.45rem 0 0;
  color: var(--ink-soft);
}

.admin-problems-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 0.75rem;
  align-items: center;
  justify-content: end;
}

@media (max-width: 1100px) {
  .admin-problems-toolbar {
    flex-direction: column;
  }

  .admin-problems-actions {
    justify-content: flex-start;
  }
}
</style>
