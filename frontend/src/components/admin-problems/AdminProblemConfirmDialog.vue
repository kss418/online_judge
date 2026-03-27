<template>
  <Teleport to="body">
    <div
      v-if="open"
      class="admin-problem-delete-backdrop"
      @click.self="$emit('close')"
    >
      <section
        class="admin-problem-delete-dialog"
        role="dialog"
        aria-modal="true"
        :aria-labelledby="titleId"
      >
        <div class="admin-problem-delete-header">
          <div>
            <p class="panel-kicker">{{ kicker }}</p>
            <h3 :id="titleId">{{ title }}</h3>
            <p class="auth-dialog-copy">
              {{ description }}
            </p>
          </div>
          <button
            type="button"
            class="icon-button"
            aria-label="닫기"
            :disabled="isBusy"
            @click="$emit('close')"
          >
            ×
          </button>
        </div>

        <div v-if="summaryLabel" class="admin-problem-delete-summary">
          <strong>{{ summaryLabel }}</strong>
          <p
            v-if="summaryCopy"
            class="admin-problem-confirm-copy"
          >
            {{ summaryCopy }}
          </p>
        </div>

        <div class="field-block">
          <label class="field-label" :for="`${titleId}-problem-id-confirm`">문제 번호 다시 입력</label>
          <input
            :id="`${titleId}-problem-id-confirm`"
            :value="problemIdInput"
            class="field-input"
            type="text"
            inputmode="numeric"
            :disabled="isBusy"
            placeholder="예: 1000"
            @input="$emit('update:problemIdInput', $event.target.value)"
          />
        </div>

        <div class="field-block">
          <label class="field-label" :for="`${titleId}-problem-title-confirm`">문제 제목 다시 입력</label>
          <input
            :id="`${titleId}-problem-title-confirm`"
            :value="titleInput"
            class="field-input"
            type="text"
            :disabled="isBusy"
            placeholder="문제 제목"
            @input="$emit('update:titleInput', $event.target.value)"
          />
        </div>

        <div class="dialog-actions">
          <button
            type="button"
            class="ghost-button"
            :disabled="isBusy"
            @click="$emit('close')"
          >
            취소
          </button>
          <button
            type="button"
            class="primary-button"
            :class="confirmButtonClass"
            :disabled="!canConfirm"
            @click="$emit('confirm')"
          >
            {{ confirmLabel }}
          </button>
        </div>
      </section>
    </div>
  </Teleport>
</template>

<script setup>
defineProps({
  open: {
    type: Boolean,
    required: true
  },
  titleId: {
    type: String,
    required: true
  },
  kicker: {
    type: String,
    required: true
  },
  title: {
    type: String,
    required: true
  },
  description: {
    type: String,
    required: true
  },
  summaryLabel: {
    type: String,
    default: ''
  },
  summaryCopy: {
    type: String,
    default: ''
  },
  problemIdInput: {
    type: String,
    default: ''
  },
  titleInput: {
    type: String,
    default: ''
  },
  isBusy: {
    type: Boolean,
    required: true
  },
  canConfirm: {
    type: Boolean,
    required: true
  },
  confirmLabel: {
    type: String,
    required: true
  },
  confirmButtonClass: {
    type: String,
    default: ''
  }
})

defineEmits([
  'close',
  'confirm',
  'update:problemIdInput',
  'update:titleInput'
])
</script>

<style scoped>
.admin-problem-delete-backdrop {
  position: fixed;
  inset: 0;
  z-index: 50;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 1.2rem;
  background: rgba(15, 23, 42, 0.46);
  backdrop-filter: blur(10px);
}

.admin-problem-delete-dialog {
  width: min(100%, 560px);
  display: grid;
  gap: 1rem;
  padding: 1.3rem;
  border-radius: 28px;
  border: 1px solid rgba(20, 33, 61, 0.12);
  background: rgba(255, 252, 248, 0.96);
  box-shadow: 0 36px 80px rgba(20, 33, 61, 0.22);
}

.admin-problem-delete-header {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: start;
}

.admin-problem-delete-summary {
  padding: 0.9rem 1rem;
  border-radius: 16px;
  border: 1px solid rgba(185, 28, 28, 0.16);
  background: rgba(255, 243, 243, 0.82);
  color: var(--danger);
}

.admin-problem-confirm-copy {
  margin: 0.4rem 0 0;
  color: var(--ink-soft);
}

.admin-problem-rejudge-confirm,
.admin-problem-delete-confirm {
  min-width: 8.5rem;
}

.admin-problem-rejudge-confirm {
  background: linear-gradient(135deg, #d97706, #ea580c);
  box-shadow: 0 12px 28px rgba(217, 119, 6, 0.28);
}

.admin-problem-delete-confirm {
  background: linear-gradient(135deg, #dc2626, #b91c1c);
  box-shadow: 0 12px 28px rgba(185, 28, 28, 0.24);
}
</style>
