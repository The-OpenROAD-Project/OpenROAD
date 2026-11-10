// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Assigns a triaged issue to the person Gemini selected.
//
// The selection is untrusted model output, so it is only applied when:
//  - primary_label is one of the labels applied to the issue,
//  - the login is a member of some CODEOWNERS team (allowed list), and
//  - the issue has no assignee yet.
//
// Used from actions/github-script:
//   const assign = require('./.github/scripts/triage-assign.js');
//   await assign({github, context, core, issueNumber, labels,
//                 primaryLabel, assignee, allowed});

function cleanLogin(login) {
  return String(login || '').replace(/[`@\s]/g, '');
}

function parseAllowed(allowed) {
  return new Set(String(allowed || '').split(',')
    .map((login) => login.trim().toLowerCase())
    .filter((login) => login));
}

module.exports = async ({github, context, core, issueNumber, labels,
                         primaryLabel, assignee, allowed}) => {
  const login = cleanLogin(assignee);
  if (!login) {
    core.info(`#${issueNumber}: no assignee selected.`);
    return false;
  }
  const allowedSet = parseAllowed(allowed);
  if (allowedSet.size === 0) {
    core.info(`#${issueNumber}: no assignee candidates; skipping assignment.`);
    return false;
  }
  if (!labels.includes(primaryLabel)) {
    core.warning(`#${issueNumber}: primary label '${primaryLabel}' is not ` +
      `among the applied labels; skipping assignment.`);
    return false;
  }
  if (!allowedSet.has(login.toLowerCase())) {
    core.warning(`#${issueNumber}: '${login}' is not a CODEOWNERS team ` +
      `member; skipping assignment.`);
    return false;
  }
  const {data: issue} = await github.rest.issues.get({
    owner: context.repo.owner,
    repo: context.repo.repo,
    issue_number: issueNumber,
  });
  if ((issue.assignees || []).length > 0) {
    core.info(`#${issueNumber}: already assigned; leaving as is.`);
    return false;
  }
  const {data: updated} = await github.rest.issues.addAssignees({
    owner: context.repo.owner,
    repo: context.repo.repo,
    issue_number: issueNumber,
    assignees: [login],
  });
  // addAssignees adds rather than replaces, so a human assignment made after
  // the check above wins: withdraw this one.
  const others = (updated.assignees || [])
    .filter((user) => user.login.toLowerCase() !== login.toLowerCase());
  if (others.length > 0) {
    await github.rest.issues.removeAssignees({
      owner: context.repo.owner,
      repo: context.repo.repo,
      issue_number: issueNumber,
      assignees: [login],
    });
    core.info(`#${issueNumber}: assigned concurrently to ` +
      `${others.map((user) => user.login).join(',')}; leaving as is.`);
    return false;
  }
  core.info(`#${issueNumber}: assigned ${login} (primary label ` +
    `'${primaryLabel}').`);
  return true;
};

module.exports.cleanLogin = cleanLogin;
