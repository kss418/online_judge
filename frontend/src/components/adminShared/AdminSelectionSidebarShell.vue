<template>
  <aside class="admin-selection-sidebar-shell">
    <slot name="search" />
    <slot name="before-list" />

    <div class="admin-selection-sidebar-shell__header">
      <p class="admin-selection-sidebar-shell__caption">{{ listCaption }}</p>
      <span class="admin-selection-sidebar-shell__count">{{ formatCount(itemCount) }}개</span>
    </div>

    <div v-if="isLoading" class="empty-state compact-state">
      <p>{{ loadingMessage }}</p>
    </div>

    <div v-else-if="errorMessage" class="empty-state error-state compact-state">
      <p>{{ errorMessage }}</p>
    </div>

    <div v-else-if="!itemCount" class="empty-state compact-state">
      <p>{{ emptyMessage }}</p>
    </div>

    <div v-else class="admin-selection-sidebar-shell__list">
      <slot />
    </div>
  </aside>
</template>

<script setup>
defineProps({
  listCaption: {
    type: String,
    required: true
  },
  itemCount: {
    type: Number,
    required: true
  },
  isLoading: {
    type: Boolean,
    required: true
  },
  errorMessage: {
    type: String,
    required: true
  },
  emptyMessage: {
    type: String,
    required: true
  },
  loadingMessage: {
    type: String,
    default: '목록을 불러오는 중입니다.'
  },
  formatCount: {
    type: Function,
    required: true
  }
})
</script>

<style scoped>
.admin-selection-sidebar-shell {
  display: grid;
  gap: 1rem;
  padding: 1rem;
  border: 1px solid var(--admin-workspace-shell-border);
  border-radius: 24px;
  background: var(--admin-workspace-shell-surface);
  box-shadow: var(--admin-workspace-shell-shadow);
}

.admin-selection-sidebar-shell__header {
  display: flex;
  justify-content: space-between;
  gap: 0.75rem;
  align-items: center;
}

.admin-selection-sidebar-shell__caption,
.admin-selection-sidebar-shell__count {
  margin: 0;
  color: var(--ink-soft);
}

.admin-selection-sidebar-shell__count {
  font-size: 0.92rem;
  font-weight: 700;
}

.admin-selection-sidebar-shell__list {
  display: grid;
  gap: 0.75rem;
  max-height: 65vh;
  overflow-y: auto;
  padding: 0.7rem;
  border: 1px solid var(--admin-workspace-section-border);
  border-radius: 22px;
  background: rgba(255, 255, 255, 0.72);
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.76),
    inset 0 0 0 1px rgba(255, 255, 255, 0.3);
  scrollbar-gutter: stable;
}

@media (max-width: 1100px) {
  .admin-selection-sidebar-shell__list {
    max-height: 24rem;
  }
}
</style>
