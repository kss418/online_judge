export const problemBusySection = Object.freeze({
  CREATE: 'create',
  SAVE_TITLE: 'title',
  SAVE_LIMITS: 'limits',
  SAVE_STATEMENT: 'statement',
  CREATE_SAMPLE: 'sample:create',
  DELETE_LAST_SAMPLE: 'sample:delete-last',
  UPLOAD_TESTCASE_ZIP: 'testcase:upload',
  REJUDGE: 'rejudge',
  DELETE: 'delete'
})

export function makeProblemSampleBusySection(sampleOrder){
  return `sample:${sampleOrder}`
}

export function isProblemSampleBusySection(currentBusySection, sampleOrder){
  return currentBusySection === makeProblemSampleBusySection(sampleOrder)
}
