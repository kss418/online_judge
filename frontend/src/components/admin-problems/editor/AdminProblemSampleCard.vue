<template>
  <article class="admin-sample-card">
    <div class="admin-sample-card-header">
      <div>
        <p class="panel-kicker">sample {{ formatCount(sampleDraft.sample_order) }}</p>
        <h3>예제 {{ formatCount(sampleDraft.sample_order) }}</h3>
      </div>

      <button
        v-if="isLastSample(sampleDraft.sample_order)"
        type="button"
        class="ghost-button admin-sample-delete-button"
        :disabled="!canDeleteLastSample"
        @click="$emit('delete-last-sample')"
      >
        {{ isDeletingLastSample ? '삭제 중...' : '샘플 삭제' }}
      </button>
    </div>

    <div class="admin-problem-form-grid">
      <label class="field-block">
        <span class="field-label">입력</span>
        <textarea
          v-model="sampleDraft.sample_input"
          class="admin-problem-textarea admin-sample-textarea"
          spellcheck="false"
          placeholder="빈 입력도 허용됩니다."
        />
      </label>

      <label class="field-block">
        <span class="field-label">출력</span>
        <textarea
          v-model="sampleDraft.sample_output"
          class="admin-problem-textarea admin-sample-textarea"
          spellcheck="false"
          placeholder="빈 출력도 허용됩니다."
        />
      </label>
    </div>

    <div class="admin-problem-section-actions">
      <button
        type="button"
        class="primary-button"
        :disabled="!canSaveSample(sampleDraft.sample_order)"
        @click="$emit('save-sample', sampleDraft.sample_order)"
      >
        {{ isSavingSample(sampleDraft.sample_order) ? '저장 중...' : '저장' }}
      </button>
    </div>
  </article>
</template>

<script setup>
defineProps({
  sampleDraft: {
    type: Object,
    required: true
  },
  canDeleteLastSample: {
    type: Boolean,
    required: true
  },
  isDeletingLastSample: {
    type: Boolean,
    required: true
  },
  formatCount: {
    type: Function,
    required: true
  },
  isSavingSample: {
    type: Function,
    required: true
  },
  canSaveSample: {
    type: Function,
    required: true
  },
  isLastSample: {
    type: Function,
    required: true
  }
})

defineEmits(['delete-last-sample', 'save-sample'])
</script>

<style scoped>
.admin-sample-card {
  display: grid;
  gap: 1rem;
  padding: 1rem;
  border-radius: 20px;
  border: 1px solid var(--admin-problems-nested-border);
  background: var(--admin-problems-nested-surface);
  box-shadow: var(--admin-problems-section-shadow);
}

.admin-sample-card-header {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: start;
}

.admin-problem-form-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 0.9rem;
}

.admin-problem-textarea {
  width: 100%;
  min-height: 10rem;
  padding: 1rem;
  resize: vertical;
  border-radius: 18px;
  border: 1px solid var(--line);
  background: var(--admin-problems-nested-surface);
  color: var(--ink-strong);
  font: inherit;
  line-height: 1.6;
}

.admin-problem-textarea:focus {
  outline: 2px solid rgba(217, 119, 6, 0.18);
  border-color: rgba(217, 119, 6, 0.5);
}

.admin-sample-textarea {
  min-height: 9rem;
  font-family: "SFMono-Regular", "Consolas", monospace;
}

.admin-sample-delete-button {
  color: var(--danger);
  background: rgba(254, 242, 242, 0.96);
  border-color: rgba(185, 28, 28, 0.18);
}

.admin-problem-section-actions {
  display: flex;
  justify-content: flex-end;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

@media (max-width: 1100px) {
  .admin-sample-card-header {
    flex-direction: column;
  }
}

@media (max-width: 720px) {
  .admin-problem-form-grid {
    grid-template-columns: minmax(0, 1fr);
  }

  .admin-problem-section-actions {
    justify-content: stretch;
  }
}
</style>
