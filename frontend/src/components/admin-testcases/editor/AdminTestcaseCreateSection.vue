<template>
  <article class="admin-testcases-section admin-testcases-section--append">
    <div class="panel-header">
      <div>
        <p class="panel-kicker">append</p>
        <h3>테스트케이스 추가</h3>
      </div>
    </div>

    <div class="admin-testcases-grid">
      <label class="field-block">
        <span class="field-label">입력</span>
        <textarea
          :value="newTestcaseInput"
          class="admin-testcases-textarea"
          spellcheck="false"
          :disabled="Boolean(busySection)"
          placeholder="빈 입력도 허용됩니다."
          @input="$emit('update:newTestcaseInput', $event.target.value)"
        />
      </label>

      <label class="field-block">
        <span class="field-label">출력</span>
        <textarea
          :value="newTestcaseOutput"
          class="admin-testcases-textarea"
          spellcheck="false"
          :disabled="Boolean(busySection)"
          placeholder="빈 출력도 허용됩니다."
          @input="$emit('update:newTestcaseOutput', $event.target.value)"
        />
      </label>
    </div>

    <div class="admin-testcases-actions">
      <button
        type="button"
        class="primary-button"
        :disabled="!canCreateTestcase"
        @click="$emit('create-testcase')"
      >
        {{ isCreatingTestcase ? '추가 중...' : '테스트케이스 추가' }}
      </button>
    </div>
  </article>
</template>

<script setup>
defineProps({
  newTestcaseInput: {
    type: String,
    default: ''
  },
  newTestcaseOutput: {
    type: String,
    default: ''
  },
  busySection: {
    type: String,
    default: ''
  },
  canCreateTestcase: {
    type: Boolean,
    required: true
  },
  isCreatingTestcase: {
    type: Boolean,
    required: true
  }
})

defineEmits([
  'update:newTestcaseInput',
  'update:newTestcaseOutput',
  'create-testcase'
])
</script>

<style scoped>
.admin-testcases-section {
  display: grid;
  gap: 1rem;
  padding: 1.2rem;
  border: 1px solid var(--admin-testcases-section-border);
  border-radius: 20px;
  background: var(--admin-testcases-section-surface);
  box-shadow: var(--admin-testcases-section-shadow);
}

.admin-testcases-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 1rem;
}

.admin-testcases-textarea {
  width: 100%;
  min-height: 12rem;
  padding: 0.9rem 1rem;
  border-radius: 18px;
  border: 1px solid var(--line);
  background: var(--admin-testcases-nested-surface);
  color: var(--ink-strong);
  font: inherit;
  resize: vertical;
  line-height: 1.6;
}

.admin-testcases-textarea:focus {
  outline: 2px solid rgba(217, 119, 6, 0.18);
  border-color: rgba(217, 119, 6, 0.5);
}

.admin-testcases-actions {
  display: flex;
  justify-content: flex-end;
  margin-top: 1rem;
}

@media (max-width: 720px) {
  .admin-testcases-grid {
    grid-template-columns: 1fr;
  }

  .admin-testcases-actions {
    justify-content: stretch;
  }
}
</style>
