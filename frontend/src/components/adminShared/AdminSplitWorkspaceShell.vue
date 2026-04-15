<template>
  <section class="page-grid single-column">
    <article class="panel admin-split-workspace-shell">
      <slot name="toolbar" />

      <div
        v-if="accessState !== 'allowed'"
        class="empty-state"
        :class="{ 'error-state': accessState === 'denied' }"
      >
        <p>{{ accessMessage }}</p>
      </div>

      <div v-else class="admin-split-workspace-shell__layout">
        <slot name="sidebar" />
        <slot />
      </div>
    </article>
  </section>
</template>

<script setup>
defineProps({
  accessState: {
    type: String,
    default: 'initializing'
  },
  accessMessage: {
    type: String,
    default: ''
  }
})
</script>

<style scoped>
.admin-split-workspace-shell {
  display: grid;
  gap: 1rem;
  border: 1px solid var(--admin-workspace-shell-border);
  background: var(--admin-workspace-shell-surface);
  box-shadow: var(--admin-workspace-shell-shadow);
  --admin-workspace-shell-surface: linear-gradient(
    180deg,
    rgba(246, 248, 251, 0.98),
    rgba(239, 243, 248, 0.94)
  );
  --admin-workspace-shell-border: rgba(148, 163, 184, 0.18);
  --admin-workspace-section-surface: linear-gradient(
    180deg,
    rgba(255, 255, 255, 0.98),
    rgba(248, 250, 252, 0.95)
  );
  --admin-workspace-section-border: rgba(148, 163, 184, 0.12);
  --admin-workspace-nested-surface: rgba(255, 255, 255, 0.98);
  --admin-workspace-nested-border: rgba(148, 163, 184, 0.14);
  --admin-workspace-shell-shadow:
    0 18px 36px rgba(20, 33, 61, 0.06),
    inset 0 1px 0 rgba(255, 255, 255, 0.7);
  --admin-workspace-section-shadow:
    0 12px 28px rgba(20, 33, 61, 0.04),
    inset 0 1px 0 rgba(255, 255, 255, 0.76);
  --admin-problems-shell-surface: var(--admin-workspace-shell-surface);
  --admin-problems-shell-border: var(--admin-workspace-shell-border);
  --admin-problems-section-surface: var(--admin-workspace-section-surface);
  --admin-problems-section-border: var(--admin-workspace-section-border);
  --admin-problems-nested-surface: var(--admin-workspace-nested-surface);
  --admin-problems-nested-border: var(--admin-workspace-nested-border);
  --admin-problems-shell-shadow: var(--admin-workspace-shell-shadow);
  --admin-problems-section-shadow: var(--admin-workspace-section-shadow);
  --admin-testcases-shell-surface: var(--admin-workspace-shell-surface);
  --admin-testcases-shell-border: var(--admin-workspace-shell-border);
  --admin-testcases-section-surface: var(--admin-workspace-section-surface);
  --admin-testcases-section-border: var(--admin-workspace-section-border);
  --admin-testcases-nested-surface: var(--admin-workspace-nested-surface);
  --admin-testcases-nested-border: var(--admin-workspace-nested-border);
  --admin-testcases-shell-shadow: var(--admin-workspace-shell-shadow);
  --admin-testcases-section-shadow: var(--admin-workspace-section-shadow);
}

.admin-split-workspace-shell__layout {
  display: grid;
  grid-template-columns: minmax(320px, 380px) minmax(0, 1fr);
  gap: 1rem;
  align-items: start;
}

@media (max-width: 1100px) {
  .admin-split-workspace-shell__layout {
    grid-template-columns: minmax(0, 1fr);
  }
}
</style>
