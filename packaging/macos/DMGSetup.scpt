-- StreamDAB Analyser — DMG setup script (used by CPack DragNDrop)
-- Simple, failure-tolerant window layout for the .dmg volume
-- "StreamDAB Analyser". Every decorative step is guarded so a missing
-- asset (background, README/LICENSE) never aborts the packaging run.

tell application "Finder"
    tell disk "StreamDAB Analyser"
        open

        -- window settings
        set current view of container window to icon view
        set toolbar visible of container window to false
        set statusbar visible of container window to false

        -- window size and position
        set the bounds of container window to {400, 100, 900, 400}

        -- icon view options
        set theViewOptions to the icon view options of container window
        set arrangement of theViewOptions to not arranged
        set icon size of theViewOptions to 72

        -- optional background (not shipped; keep the default look)
        try
            set background picture of theViewOptions to file ".background:background.png"
        on error
            -- no background asset packaged; keep default
        end try

        -- application icon
        set position of item "StreamDAB-Analyser.app" of container window to {150, 175}

        -- Applications folder link for installation
        set position of item "Applications" of container window to {350, 175}

        -- optional documentation/license positions
        try
            set position of item "README.md" of container window to {250, 275}
        on error
        end try
        try
            set position of item "LICENSE" of container window to {150, 275}
        on error
        end try

        -- update and close
        update without registering applications
        delay 1
        close
    end tell
end tell