# QML modules, from bottom to top layer (see docs/UI-Development-Plan.md):
#   Calibly.Theme       colors, spacing, fonts, icons
#   Calibly.Controls    neutral building blocks
#   Calibly.Components  domain building blocks
#   Calibly.Views       screens
# Each layer may only use the layers below it.
add_subdirectory(Theme)
add_subdirectory(Controls)
add_subdirectory(Components)
add_subdirectory(Views)
