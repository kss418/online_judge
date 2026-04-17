<template>
  <Teleport to="body">
    <div
      v-if="dialog.model.open"
      class="admin-problem-delete-backdrop"
      @click.self="dialog.actions.close()"
    >
      <section
        class="admin-problem-delete-dialog"
        role="dialog"
        aria-modal="true"
        :aria-labelledby="dialog.model.titleId"
      >
        <div class="admin-problem-delete-header">
          <div>
            <p class="panel-kicker">{{ dialog.model.kicker }}</p>
            <h3 :id="dialog.model.titleId">{{ dialog.model.title }}</h3>
            <p class="auth-dialog-copy">
              {{ dialog.model.description }}
            </p>
          </div>
          <button
            type="button"
            class="icon-button"
            aria-label="닫기"
            :disabled="dialog.model.isBusy"
            @click="dialog.actions.close()"
          >
            ×
          </button>
        </div>

        <div v-if="dialog.model.summaryLabel" class="admin-problem-delete-summary">
          <strong>{{ dialog.model.summaryLabel }}</strong>
          <p
            v-if="dialog.model.summaryCopy"
            class="admin-problem-confirm-copy"
          >
            {{ dialog.model.summaryCopy }}
          </p>
        </div>

        <div class="field-block">
          <label class="field-label" :for="`${dialog.model.titleId}-problem-id-confirm`">문제 번호 다시 입력</label>
          <input
            :id="`${dialog.model.titleId}-problem-id-confirm`"
            :value="dialog.model.problemIdInput"
            class="field-input"
            type="text"
            inputmode="numeric"
            :disabled="dialog.model.isBusy"
            :placeholder="dialog.model.problemIdPlaceholder"
            @input="handleProblemIdInput"
          />
        </div>

        <div class="field-block">
          <label class="field-label" :for="`${dialog.model.titleId}-problem-title-confirm`">문제 제목 다시 입력</label>
          <input
            :id="`${dialog.model.titleId}-problem-title-confirm`"
            :value="dialog.model.titleInput"
            class="field-input"
            type="text"
            :disabled="dialog.model.isBusy"
            :placeholder="dialog.model.titlePlaceholder"
            @input="handleTitleInput"
          />
        </div>

        <div class="dialog-actions">
          <button
            type="button"
            class="ghost-button"
            :disabled="dialog.model.isBusy"
            @click="dialog.actions.close()"
          >
            취소
          </button>
          <button
            type="button"
            class="primary-button"
            :class="dialog.model.confirmButtonClass"
            :disabled="!dialog.model.canConfirm"
            @click="dialog.actions.confirm()"
          >
            {{ dialog.model.confirmLabel }}
          </button>
        </div>
      </section>
    </div>
  </Teleport>
</template>

<script setup>
const props = defineProps({
  dialog: {
    type: Object,
    required: true
  }
})

function handleProblemIdInput(event){
  props.dialog.actions.updateProblemIdInput(event.target.value)
}

function handleTitleInput(event){
  props.dialog.actions.updateTitleInput(event.target.value)
}
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
