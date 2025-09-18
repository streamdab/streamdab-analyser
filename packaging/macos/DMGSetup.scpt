-- ETI Stream Analyzer Professional DMG Setup Script
-- Professional macOS disk image layout and styling

tell application "Finder"
    tell disk "ETI Stream Analyzer"
        open

        -- Professional window settings
        set current view of container window to icon view
        set toolbar visible of container window to false
        set statusbar visible of container window to false

        -- Professional window size and position
        set the bounds of container window to {400, 100, 900, 400}

        -- Professional icon view settings
        set theViewOptions to the icon view options of container window
        set arrangement of theViewOptions to not arranged
        set icon size of theViewOptions to 72
        set background picture of theViewOptions to file ".background:background.png"

        -- Professional icon positioning for broadcast industry appeal
        -- Application icon positioned for easy access
        set position of item "ETI Stream Analyzer.app" of container window to {150, 175}

        -- Applications folder link for professional installation
        set position of item "Applications" of container window to {350, 175}

        -- Professional documentation positioning
        if exists item "README.txt" then
            set position of item "README.txt" of container window to {250, 275}
        end if

        -- Professional license positioning
        if exists item "LICENSE.txt" then
            set position of item "LICENSE.txt" of container window to {150, 275}
        end if

        -- Professional samples folder positioning
        if exists item "Samples" then
            set position of item "Samples" of container window to {350, 275}
        end if

        -- Update and close for professional appearance
        update without registering applications
        delay 2
        close
    end tell
end tell
