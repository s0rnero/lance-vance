#!/usr/bin/env node
/**
 * verify.mjs - Health check of the harness installed in a repository.
 *
 * Validates:
 *   - manifest.json present with version/origin
 *   - core files present
 *   - per-repo files present (warning if missing: they need adaptation)
 *   - plans: valid frontmatter, status within the lifecycle, created date
 *   - decisions (ADR): frontmatter and valid status
 *   - generated root compatibility layer
 *   - installed skills (skills-lock.json)
 *
 * Usage:
 *   node scripts/verify.mjs <repo>    # exit 0 when there are no errors, 1 otherwise
 *   node scripts/verify.mjs           # uses the current directory
 */
import fs from 'node:fs';
import path from 'node:path';

const target = path.resolve(process.argv[2] || process.cwd());
const agentsDir = path.join(target, '.agents');
const errors = [];
const warnings = [];
const infos = [];

function parseFrontmatter(p) {
  const c = fs.readFileSync(p, 'utf8');
  const m = c.match(/^---\n([\s\S]*?)\n---/);
  if (!m) return null;
  const out = {};
  for (const line of m[1].split('\n')) {
    const i = line.indexOf(':');
    if (i === -1) continue;
    const k = line.slice(0, i).trim();
    const v = line.slice(i + 1).trim().replace(/^["']|["']$/g, '');
    if (k) out[k] = v;
  }
  return out;
}

const manifestPath = path.join(agentsDir, 'manifest.json');
let manifest = null;
try { manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8')); } catch { manifest = null; }

if (!manifest) {
  errors.push('No harness installed: .agents/manifest.json is missing (run scripts/setup.mjs)');
} else {
  infos.push(`Harness: ${manifest.name || '?'} v${manifest.version || '?'}`);
  if (manifest.origin)  infos.push(`Origin: ${manifest.origin}`);

  for (const rel of manifest.core || []) {
    if (!fs.existsSync(path.join(target, rel))) errors.push(`Missing core file: ${rel}`);
  }
  for (const rel of manifest.perRepo || []) {
    if (!fs.existsSync(path.join(target, rel))) warnings.push(`Missing per-repo file (create/adapt): ${rel}`);
  }

  const ALLOWED = ['CREATED', 'ENRICHED', 'EXECUTED'];
  const plansDir = path.join(agentsDir, 'plans');
  if (fs.existsSync(plansDir)) {
    const plans = fs.readdirSync(plansDir).filter((f) => f.endsWith('.md') && f !== 'README.md');
    if (plans.length === 0) infos.push('Plans: no plans created yet');
    for (const f of plans) {
      const fm = parseFrontmatter(path.join(plansDir, f));
      if (!fm) { errors.push(`Plan without valid frontmatter: plans/${f}`); continue; }
      if (!fm.name) warnings.push(`Plan without name: plans/${f}`);
      if (!fm.status) errors.push(`Plan without status: plans/${f}`);
      else if (!ALLOWED.includes(fm.status)) errors.push(`Plan with invalid status (${fm.status}): plans/${f}`);
      if (fm.created && !/^\d{4}-\d{2}-\d{2}/.test(fm.created)) warnings.push(`Plan with invalid created date: plans/${f}`);
      if (!/^\d{4}-\d{2}-\d{2}-\d+\.\d+-.+\.md$/.test(f)) warnings.push(`Plan outside naming convention (YYYY-MM-DD-<fam>.<iter>-<slug>.md): plans/${f}`);
    }
  } else {
    warnings.push('.agents/plans/ directory missing');
  }

  const decisionsDir = path.join(agentsDir, 'decisions');
  if (fs.existsSync(decisionsDir)) {
    for (const f of fs.readdirSync(decisionsDir)) {
      if (!f.endsWith('.md') || f === 'README.md') continue;
      const fm = parseFrontmatter(path.join(decisionsDir, f));
      if (!fm) { warnings.push(`ADR without valid frontmatter: decisions/${f}`); continue; }
      if (fm.status && !['proposed', 'accepted', 'superseded'].includes(fm.status)) {
        warnings.push(`ADR with invalid status (${fm.status}): decisions/${f}`);
      }
    }
  } else {
    warnings.push('.agents/decisions/ directory missing (optional)');
  }

  for (const rel of manifest.generated || []) {
    if (!fs.existsSync(path.join(target, rel))) warnings.push(`Missing root compat file: ${rel} (regenerate with setup update)`);
  }

  const lock = path.join(agentsDir, 'skills', 'skills-lock.json');
  if (fs.existsSync(lock)) infos.push('Skills: installed (skills-lock.json present)');
  else infos.push('Skills: no skills-lock.json (run npx autoskills -y)');
}

console.log(`\n==> Verify harness at: ${target}\n`);
for (const i of infos) console.log(`  [info]  ${i}`);
for (const w of warnings) console.log(`  [warn]  ${w}`);
for (const e of errors) console.log(`  [error] ${e}`);
console.log(`\n  ${errors.length} errors, ${warnings.length} warnings, ${infos.length} infos`);
process.exit(errors.length ? 1 : 0);
