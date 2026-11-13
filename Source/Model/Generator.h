#pragma once
#include "Plan.h"

namespace choreo
{
// Plan → Animator-Szene. Eigenständige Gruppen werden Abschnitt für Abschnitt aus ihren Gesten
// gebaut; abgeleitete Gruppen (Unisono, Kanon, Spiegelung, Umkehrung) entstehen aus den fertigen
// Clips ihrer Leitgruppe, geometrisch transformiert und zeitlich versetzt.
Scene planToScene(Plan& plan);

// Prüfung inklusive Plan-Notizen (wie collectIssues der Web-App)
std::vector<Issue> collectPlanIssues(const Plan&, const Scene&);
} // namespace choreo
