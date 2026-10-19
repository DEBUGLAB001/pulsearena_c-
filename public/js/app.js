const $ = (id) => document.getElementById(id);
let currentState = null;
let currentVersion = null;
let busy = false;

const initials = (id) => id.slice(0, 2);
const teamName = (id, state) => {
  for (const group of state.groups) for (const team of group.teams) if (team.id === id) return team.name;
  return id.startsWith('winner-') ? 'Winner · ' + id.slice(7) : id || 'TBD';
};

function renderStats(state) {
  const matchesPlayed = state.matches.filter(m => m.played).length;
  const all = state.groups.flatMap(g => g.teams);
  const leader = state.leader;
  const cards = [
    ['TEAMS IN PLAY', all.length, 'Across qualification groups', '#69d9e8'],
    ['MATCHES PLAYED', `${matchesPlayed} / ${state.matchCount}`, 'Group stage fixtures', '#a9f36c'],
    ['PLAYOFF PLACES', state.qualifierCount, 'Top teams advance', '#b09cff'],
    ['CURRENT LEADER', leader?.id ?? '—', leader ? `${leader.points} pts · ${leader.diff > 0 ? '+' : ''}${leader.diff} diff` : 'Waiting for results', '#ffb86b']
  ];
  $('stats').innerHTML = cards.map(([label, value, foot, color]) => `<div class="stat-card" style="--glow:${color}"><div class="stat-label">${label}</div><div class="stat-value">${value}</div><div class="stat-foot">${foot}</div></div>`).join('');
}

function renderGroups(state) {
  $('groupsContent').innerHTML = state.groups.map(group => `<div class="group-card"><div class="group-title">Group ${group.name}<span>${group.teams.filter((_, i) => i < 2).length} QUALIFY</span></div>${group.teams.map((team, i) => `<div class="team-row ${i < 2 ? 'qualify' : ''}"><span class="seed">${String(i + 1).padStart(2, '0')}</span><span class="team-name" title="${team.name}">${team.name}</span><span class="team-meta">${team.wins}W</span><span class="team-pts">${team.points} P</span></div>`).join('')}</div>`).join('');
}

function renderMatches(state) {
  const filter = $('groupFilter').value;
  const matches = state.matches.filter(m => filter === 'all' || m.group === filter);
  $('matchesContent').innerHTML = matches.map((match, i) => {
    const home = teamName(match.home, state), away = teamName(match.away, state);
    return `<div class="match-card"><div class="match-time">${match.group}<br>${String(i + 1).padStart(2, '0')} : 30</div><div class="match-pair"><div class="match-team"><strong>${home}</strong><em>${match.played ? match.homeScore : '—'}</em></div><div class="versus">VS · ${match.id}</div><div class="match-team"><strong>${away}</strong><em>${match.played ? match.awayScore : '—'}</em></div></div><button class="match-action ${match.played ? 'done' : ''}" data-match="${match.id}" data-winner="${match.home}">${match.played ? '↻ Retry result' : 'Record result'}</button></div>`;
  }).join('') || '<div class="subtle">No fixtures in this group.</div>';
  document.querySelectorAll('[data-match]').forEach(button => button.addEventListener('click', () => submit(button.dataset.match, button.dataset.winner)));
}

function renderBracket(state) {
  const rounds = ['Opening round', 'Semifinal', 'Final'];
  $('bracketContent').innerHTML = rounds.map(round => {
    const slots = state.bracket.filter(slot => slot.round === round);
    return `<div class="round-col"><div class="round-name">${round}</div>${slots.map(slot => {
      const a = teamName(slot.teamA, state), b = teamName(slot.teamB, state);
      return `<div class="bracket-match ${slot.byeA ? 'bye' : ''}"><div class="bracket-id"><span>${slot.matchId}</span>${slot.byeA ? '<span class="bye-label">BYE ADVANTAGE</span>' : ''}</div><div class="bracket-team ${slot.byeA ? 'seeded' : ''}"><span>${a}</span><span>${slot.winner === slot.teamA ? '✓' : ''}</span></div><div class="bracket-team"><span>${b}</span><span>${slot.winner === slot.teamB ? '✓' : ''}</span></div></div>`;
    }).join('') || '<div class="subtle">Bracket updates as teams qualify.</div>'}</div>`;
  }).join('');
}

function render(state) {
  currentState = state;
  renderStats(state); renderGroups(state); renderMatches(state); renderBracket(state);
  $('syncTime').textContent = new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
  $('connection').textContent = '';
}

async function refresh() {
  if (busy) return;
  busy = true;
  try {
    const response = await fetch('api/state', { cache: 'no-store' });
    if (!response.ok) throw new Error('State request failed');
    render(await response.json());
  } catch (error) {
    $('connection').textContent = 'Reconnecting to tournament service…';
  } finally { busy = false; }
}

async function checkVersion() {
  try {
    const response = await fetch('api/version', { cache: 'no-store' });
    const data = await response.json();
    if (currentVersion === null) currentVersion = data.version;
    else if (currentVersion !== data.version) location.reload();
  } catch (_) { /* Retry on the next poll; backend restarts are brief. */ }
}

async function submit(matchId, winner) {
  try {
    const body = new URLSearchParams({ matchId, winner });
    const response = await fetch('api/result', { method: 'POST', body, cache: 'no-store' });
    if (!response.ok) throw new Error('Could not record this result');
    await refresh();
  } catch (error) { $('connection').textContent = error.message; }
}

$('refreshBtn').addEventListener('click', refresh);
$('resetBtn').addEventListener('click', async () => { await fetch('api/reset', { method: 'POST', cache: 'no-store' }); await refresh(); });
$('groupFilter').addEventListener('change', () => currentState && renderMatches(currentState));
refresh(); checkVersion();
setInterval(refresh, 1800);
setInterval(checkVersion, 2200);
