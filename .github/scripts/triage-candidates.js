// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Builds the assignee candidate table for Gemini issue triage.
//
// Each repository label resolves to the CODEOWNERS team(s) that own a
// representative path for it.  The table lists, per label, the members of
// those teams with:
//  - area_assigned: how many of the last 100 issues and pull requests with
//    that label (issues only counted) are assigned to them, which reflects
//    how maintainers triage the area;
//  - open_assigned: their open assigned issues across the repository;
//  - recent_commits: their commits among the module's last 100 commits in
//    the past year.
// and `suggested`, the member the assignment rule picks (suggestAssignee).
//
// Used from actions/github-script in the job that runs Gemini:
//   const run = require('./.github/scripts/triage-candidates.js');
//   await run({github, context, core});
//
// Exports the masked table as the environment variable ASSIGNEE_CANDIDATES
// (empty on failure) and sets the step output `built` to 'true' or 'false'.
// A masked value cannot be passed between jobs, so the job that applies the
// assignment calls allowedAssignees() itself, and assigns only when `built`
// is 'true'.

const fs = require('fs');

// Labels without a /src/<module>/ directory, mapped to a path whose
// CODEOWNERS entry owns that area.
const kLabelPaths = {
  'bazel': '/MODULE.bazel',
  'bazel-conversion-must': '/bazel/x',
  'docker': '/Dockerfile',
  'github_actions': '/.github/workflows/x',
  'dependencies': '/third-party/x',
  'submodules': '/third-party/x',
};

const kFallbackKey = '*';
const kCommitHistoryDays = 365;
const kCommitSampleSize = 100;
const kHistoryIssues = 100;
// Parallel API calls; well under GitHub's secondary rate limit.
const kConcurrency = 8;

// Converts a CODEOWNERS pattern into an anchored, case-insensitive RegExp.
// Supports the forms used in this repository: '*', '/dir/', '/file', and
// '/prefix*'.
function patternToRegExp(pattern) {
  if (pattern === '*') {
    return /^.*$/;
  }
  let body = pattern.replace(/[.+?^${}()|[\]\\]/g, '\\$&')
    .replace(/\*/g, '[^/]*');
  if (!body.startsWith('/')) {
    // Unanchored patterns match at any depth.
    body = '(?:.*/)?' + body;
  }
  // A directory pattern matches everything below it; a file pattern also
  // matches a directory of that name.
  body = body.endsWith('/') ? body + '.*' : body + '(?:/.*)?';
  return new RegExp('^' + body + '$', 'i');
}

// Returns [{regexp, teams: ['org/team', ...]}] in file order.
function parseCodeowners(text) {
  const rules = [];
  for (const rawLine of text.split('\n')) {
    const line = rawLine.replace(/#.*/, '').trim();
    if (!line) {
      continue;
    }
    const [pattern, ...owners] = line.split(/\s+/);
    const teams = owners
      .filter((owner) => owner.startsWith('@') && owner.includes('/'))
      .map((owner) => owner.slice(1));
    rules.push({pattern, regexp: patternToRegExp(pattern), teams});
  }
  return rules;
}

// CODEOWNERS semantics: the last matching rule wins.
function ownersForPath(rules, path) {
  let match = null;
  for (const rule of rules) {
    if (rule.regexp.test(path)) {
      match = rule;
    }
  }
  return match ? match.teams : [];
}

// Returns {label: {path, module}} where module is the src/ directory name
// (with the CODEOWNERS spelling) or null.
function resolveLabels(rules, labels) {
  const modules = new Map();
  for (const rule of rules) {
    const m = rule.pattern.match(/^\/src\/([^/*]+)\/$/);
    if (m) {
      modules.set(m[1].toLowerCase(), m[1]);
    }
  }
  const resolved = {};
  for (const label of labels) {
    const module = modules.get(label.toLowerCase());
    if (module) {
      resolved[label] = {path: `/src/${module}/x`, module};
    } else if (kLabelPaths[label.toLowerCase()]) {
      resolved[label] = {path: kLabelPaths[label.toLowerCase()], module: null};
    }
  }
  return resolved;
}

// Runs fn over items with at most `limit` calls in flight, keeping order.
async function mapLimit(items, limit, fn) {
  const results = new Array(items.length);
  let next = 0;
  const worker = async () => {
    while (next < items.length) {
      const i = next++;
      results[i] = await fn(items[i]);
    }
  };
  await Promise.all(Array.from({length: Math.min(limit, items.length)}, worker));
  return results;
}

// Returns {team: [login, ...]}.  A team that cannot be read (renamed,
// deleted, or not visible to the token) is skipped with a warning so the
// other teams still produce candidates.
async function teamMembers(github, core, teams) {
  const lists = await mapLimit(teams, kConcurrency, async (team) => {
    const [org, slug] = team.split('/');
    try {
      const list = await github.paginate(github.rest.teams.listMembersInOrg, {
        org,
        team_slug: slug,
        per_page: 100,
      });
      return list.map((user) => user.login).sort();
    } catch (err) {
      core.warning(`Skipping team ${team}: ${err.message}`);
      return [];
    }
  });
  const members = {};
  teams.forEach((team, i) => {
    members[team] = lists[i];
  });
  return members;
}

async function openAssignedCounts(github, context) {
  const counts = {};
  const issues = await github.paginate(github.rest.issues.listForRepo, {
    owner: context.repo.owner,
    repo: context.repo.repo,
    state: 'open',
    assignee: '*',
    per_page: 100,
  });
  for (const issue of issues) {
    if (issue.pull_request) {
      continue;
    }
    for (const assignee of issue.assignees || []) {
      counts[assignee.login] = (counts[assignee.login] || 0) + 1;
    }
  }
  return counts;
}

// Counts who maintainers assigned recent issues with this label to.
// Returns {login: count}.
async function areaAssignedCounts(github, context, label) {
  const {data: issues} = await github.rest.issues.listForRepo({
    owner: context.repo.owner,
    repo: context.repo.repo,
    labels: label,
    state: 'all',
    sort: 'created',
    direction: 'desc',
    per_page: kHistoryIssues,
  });
  const counts = {};
  for (const issue of issues) {
    if (issue.pull_request) {
      continue;
    }
    for (const assignee of issue.assignees || []) {
      counts[assignee.login] = (counts[assignee.login] || 0) + 1;
    }
  }
  return counts;
}

async function recentCommitCounts(github, context, module, candidates) {
  const since = new Date(Date.now() - kCommitHistoryDays * 86400 * 1000);
  // One page is enough to show who is active, and keeps the run short.
  const {data: commits} = await github.rest.repos.listCommits({
    owner: context.repo.owner,
    repo: context.repo.repo,
    path: `src/${module}`,
    since: since.toISOString(),
    per_page: kCommitSampleSize,
  });
  const counts = {};
  for (const commit of commits) {
    // Merge commits are authored by whoever merged, not the code author.
    if ((commit.parents || []).length > 1) {
      continue;
    }
    const login = commit.author && commit.author.login;
    if (login && candidates.has(login)) {
      counts[login] = (counts[login] || 0) + 1;
    }
  }
  return counts;
}

// The assignment rule spreads work among the people who handle an area.
// The pool is the members with area_assigned above 0, else those with
// recent_commits above 0, else all of them. From the pool, the member with
// the fewest open_assigned wins; ties go to the higher area_assigned, then
// the higher recent_commits, then the login, so the pick is deterministic.
function suggestAssignee(members) {
  let pool = members.filter((m) => m.area_assigned > 0);
  if (pool.length === 0) {
    pool = members.filter((m) => m.recent_commits > 0);
  }
  if (pool.length === 0) {
    pool = members;
  }
  const best = [...pool].sort((a, b) =>
    a.open_assigned - b.open_assigned ||
    b.area_assigned - a.area_assigned ||
    b.recent_commits - a.recent_commits ||
    a.login.localeCompare(b.login))[0];
  return best ? best.login : '';
}

async function buildCandidates({github, context, core, labels, codeowners}) {
  const rules = parseCodeowners(codeowners);
  const resolved = resolveLabels(rules, labels);

  const labelTeams = {};
  for (const [label, {path}] of Object.entries(resolved)) {
    labelTeams[label] = ownersForPath(rules, path);
  }
  labelTeams[kFallbackKey] = ownersForPath(rules, '/README.md');

  const allTeams = [...new Set(rules.flatMap((rule) => rule.teams))].sort();
  const members = await teamMembers(github, core, allTeams);
  const allowed = new Set(Object.values(members).flat());
  const workload = await openAssignedCounts(github, context);

  const entries = Object.entries(labelTeams)
    .map(([label, teams]) => ({
      label,
      teams,
      module: resolved[label] ? resolved[label].module : null,
      logins: [...new Set(teams.flatMap((team) => members[team] || []))].sort(),
    }))
    .filter((entry) => entry.logins.length > 0);
  const rows = await mapLimit(entries, kConcurrency, async (entry) => {
    const assigned = entry.label === kFallbackKey ?
      {} : await areaAssignedCounts(github, context, entry.label);
    const commits = entry.module ? await recentCommitCounts(
      github, context, entry.module, new Set(entry.logins)) : {};
    const rowMembers = entry.logins.map((login) => ({
      login,
      area_assigned: assigned[login] || 0,
      open_assigned: workload[login] || 0,
      recent_commits: commits[login] || 0,
    }));
    return {
      teams: entry.teams.map((team) => team.split('/')[1]),
      suggested: suggestAssignee(rowMembers),
      members: rowMembers,
    };
  });
  const table = {};
  entries.forEach((entry, i) => {
    table[entry.label] = rows[i];
  });
  return {table, allowed: [...allowed].sort()};
}

async function repoLabels(github, context) {
  const labels = await github.paginate(github.rest.issues.listLabelsForRepo, {
    owner: context.repo.owner,
    repo: context.repo.repo,
    per_page: 100,
  });
  return labels.map((label) => label.name);
}

// Returns the logins of every CODEOWNERS team member, the only people
// triage may assign.
async function allowedAssignees({github, core}) {
  const rules = parseCodeowners(fs.readFileSync('.github/CODEOWNERS', 'utf8'));
  const teams = [...new Set(rules.flatMap((rule) => rule.teams))].sort();
  const members = await teamMembers(github, core, teams);
  return [...new Set(Object.values(members).flat())].sort();
}

module.exports = async ({github, context, core}) => {
  let json = '';
  try {
    const labels = await repoLabels(github, context);
    const codeowners = fs.readFileSync('.github/CODEOWNERS', 'utf8');
    const {table, allowed} = await buildCandidates({
      github, context, core, labels, codeowners,
    });
    json = JSON.stringify(table);
    core.info(`Assignee candidates for ${Object.keys(table).length} labels ` +
      `(${json.length} bytes), ${allowed.length} allowed assignees.`);
  } catch (err) {
    // Assignment is best effort: labeling continues without candidates.
    core.warning(`Could not build assignee candidates: ${err.message}`);
  }
  // The table shows who is on each team, which teams keep visible only to
  // org members, so mask it before it can appear in a public log.
  if (json) {
    core.setSecret(json);
  }
  core.exportVariable('ASSIGNEE_CANDIDATES', json);
  // Unlike the table, a plain flag can pass to the job that assigns.
  core.setOutput('built', json ? 'true' : 'false');
};

module.exports.parseCodeowners = parseCodeowners;
module.exports.ownersForPath = ownersForPath;
module.exports.resolveLabels = resolveLabels;
module.exports.buildCandidates = buildCandidates;
module.exports.allowedAssignees = allowedAssignees;
module.exports.suggestAssignee = suggestAssignee;
