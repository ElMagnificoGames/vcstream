import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: root
    width: 960
    height: 540
    minimumWidth: uiMetrics.minimumWindowWidth
    minimumHeight: uiMetrics.minimumWindowHeight
    visible: true
    title: "VCStream"

    onWidthChanged: {
        if ( width < minimumWidth ) {
            width = minimumWidth
        }
    }

    onHeightChanged: {
        if ( height < minimumHeight ) {
            height = minimumHeight
        }
    }

    QtObject {
        id: uiMetrics
        objectName: "uiMetrics"

        readonly property int landingPageMinWidth: 360
        readonly property int landingPageMinHeight: 420

        readonly property int shellLeftPaneMinWidth: 220
        readonly property int shellCentrePaneMinWidth: 320
        readonly property int shellRightPaneMinWidth: 260
        readonly property int shellPageMinWidth: shellLeftPaneMinWidth + shellCentrePaneMinWidth + shellRightPaneMinWidth
        readonly property int shellPageMinHeight: 540

        readonly property int minimumWindowWidth: Math.max( landingPageMinWidth, shellPageMinWidth )
        readonly property int minimumWindowHeight: Math.max( landingPageMinHeight, shellPageMinHeight )
    }

    SystemPalette {
        id: systemPal
    }

    QtObject {
        id: uiTheme

        readonly property string styleFamily: "workshop"
        readonly property string mode: ( appSupervisor && appSupervisor.preferences ? appSupervisor.preferences.themeMode : "victorian" )
        readonly property string accent: ( appSupervisor && appSupervisor.preferences ? appSupervisor.preferences.accent : "victorian" )

        readonly property string fontPreset: ( appSupervisor && appSupervisor.preferences ? appSupervisor.preferences.fontPreset : "victorian" )
        readonly property string customFontFamily: ( appSupervisor && appSupervisor.preferences ? appSupervisor.preferences.fontFamily : "" )
        readonly property int fontScalePercent: ( appSupervisor && appSupervisor.preferences ? appSupervisor.preferences.fontScalePercent : 100 )
        readonly property string density: ( appSupervisor && appSupervisor.preferences ? appSupervisor.preferences.density : "comfortable" )
        readonly property int zoomPercent: ( appSupervisor && appSupervisor.preferences ? appSupervisor.preferences.zoomPercent : 100 )

        readonly property string victorianBodyFontFamily: ( appSupervisor ? appSupervisor.victorianBodyFontFamily : "" )
        readonly property string victorianHeadingFontFamily: ( appSupervisor ? appSupervisor.victorianHeadingFontFamily : "" )

        readonly property string bodyFontFamily: {
            if ( fontPreset === "custom" && customFontFamily && customFontFamily.length > 0 ) {
                return customFontFamily
            }

            if ( fontPreset === "victorian" && victorianBodyFontFamily && victorianBodyFontFamily.length > 0 ) {
                return victorianBodyFontFamily
            }

            return Qt.application.font.family
        }

        readonly property string headingFontFamily: {
            if ( fontPreset === "victorian" && victorianHeadingFontFamily && victorianHeadingFontFamily.length > 0 ) {
                return victorianHeadingFontFamily
            }
            return bodyFontFamily
        }

        readonly property real zoomScale: Math.max( 0.5, Math.min( 2.0, zoomPercent / 100.0 ) )
        readonly property real fontScale: Math.max( 0.75, Math.min( 1.5, fontScalePercent / 100.0 ) )

        readonly property real densityScale: {
            if ( density === "compact" ) return 0.86
            if ( density === "spacious" ) return 1.22
            return 1.0
        }

        // Macro layout constraints should not follow zoom, but should respond to density.
        // Compact density means less negative space, so allow larger macro extents.
        function macroPx( basePx ) {
            return Math.max( 1, Math.round( basePx / densityScale ) )
        }

        function uiPx( basePx ) {
            return Math.max( 1, Math.round( basePx * zoomScale ) )
        }

        function spacePx( basePx ) {
            return Math.max( 1, Math.round( basePx * zoomScale * densityScale ) )
        }

        function fontPx( basePx ) {
            return Math.max( 1, Math.round( basePx * zoomScale * fontScale ) )
        }

        readonly property int baseFontPxUnscaled: {
            var px = Qt.application.font.pixelSize
            if ( px === undefined || px === null || px <= 0 ) {
                px = 14
            }
            return px
        }

        readonly property int fontBasePx: fontPx( baseFontPxUnscaled )
        readonly property int fontSmallPx: Math.max( 10, fontPx( Math.max( 10, baseFontPxUnscaled - 2 ) ) )
        readonly property int fontHeadingPx: Math.max( fontBasePx + 2, Math.round( fontBasePx * 1.28 ) )
        readonly property int fontTitlePx: Math.max( fontHeadingPx + 6, Math.round( fontBasePx * 2.0 ) )
        readonly property int fontShellTitlePx: Math.max( fontHeadingPx + 4, Math.round( fontBasePx * 1.55 ) )

        readonly property int spaceNudge: spacePx( 4 )
        readonly property int spaceCompact: spacePx( 8 )
        readonly property int spaceTight: spacePx( 12 )
        readonly property int spaceBase: spacePx( 16 )
        readonly property int spaceRelaxed: spacePx( 24 )
        readonly property int lineThin: 1
        readonly property int lineMed: 2
        readonly property int insetGap: Math.max( 2, spacePx( 3 ) )
        readonly property int panelRadius: uiPx( 8 )
        readonly property int controlRadius: uiPx( 10 )
        readonly property int panelPadding: spacePx( 14 )

        readonly property int controlHeight: uiPx( 42 )
        readonly property int compactControlHeight: uiPx( 34 )
        readonly property int toolbarHeight: uiPx( 56 )
        readonly property int statusBarHeight: uiPx( 30 )

        function blend( a, b, t ) {
            var tt = Math.max( 0.0, Math.min( 1.0, t ) )
            return Qt.rgba(
                a.r * ( 1.0 - tt ) + b.r * tt,
                a.g * ( 1.0 - tt ) + b.g * tt,
                a.b * ( 1.0 - tt ) + b.b * tt,
                1.0 )
        }

        function srgbToLinear01( u ) {
            if ( u <= 0.04045 ) {
                return u / 12.92
            }
            return Math.pow( ( u + 0.055 ) / 1.055, 2.4 )
        }

        function luminance( c ) {
            // Relative luminance, matching WCAG expectations.
            const r = srgbToLinear01( c.r )
            const g = srgbToLinear01( c.g )
            const b = srgbToLinear01( c.b )
            return 0.2126 * r + 0.7152 * g + 0.0722 * b
        }

        readonly property color anchorWhite: Qt.rgba( 1, 1, 1, 1 )
        readonly property color anchorBlack: Qt.rgba( 0, 0, 0, 1 )

        // Scholarly Gothic (Victorian) palette.
        // Kept here as tokens so authored UI remains token-driven.
        readonly property color victorianPaper: Qt.rgba( 0.953, 0.925, 0.861, 1.0 )
        readonly property color victorianPaperAlt: Qt.rgba( 0.931, 0.900, 0.830, 1.0 )
        readonly property color victorianInk: Qt.rgba( 0.094, 0.078, 0.067, 1.0 )
        // Scholarly Gothic triad (aligned with scheduleapp.css).
        readonly property color victorianNavy: "#1F2F4B"
        readonly property color victorianOxblood: "#5B1F2A"
        readonly property color victorianBrass: "#B28A2A"

        // Light mode is app-owned to avoid inheriting the OS theme's darkness.
        // Keep it warm and mostly white, with a slight paper tint.
        // Keep the paper/base very close in value to avoid a yellow-looking inner surface.
        readonly property color lightPaper: "#FCFBF8"
        readonly property color lightPaperAlt: "#FAF9F6"
        readonly property color lightInk: "#1A1613"

        // Explicit preset accents (curated, not generated from shared OKLCH LC).
        readonly property color presetRed: "#B3263A"
        readonly property color presetOrange: "#E07A2F"
        readonly property color presetYellow: "#E0B21B"
        readonly property color presetGreen: "#2FA66B"
        readonly property color presetCyan: "#24B3B6"
        readonly property color presetBlue: "#3D7BEB"
        readonly property color presetPink: "#FC89AC"
        readonly property color presetPurple: "#8F63F4"

        readonly property color windowColour: {
            if ( mode === "victorian" ) return victorianPaper
            if ( mode === "light" ) return lightPaper
            if ( mode === "dark" ) return blend( systemPal.window, anchorBlack, 0.78 )
            return systemPal.window
        }

        readonly property color baseColour: {
            if ( mode === "victorian" ) return victorianPaperAlt
            if ( mode === "light" ) return lightPaperAlt
            if ( mode === "dark" ) return blend( systemPal.base, anchorBlack, 0.72 )
            return systemPal.base
        }

        readonly property color textColour: {
            if ( mode === "victorian" ) return victorianInk
            if ( mode === "light" ) return lightInk
            if ( mode === "dark" ) return blend( systemPal.text, anchorWhite, 0.65 )
            return systemPal.text
        }

        readonly property color buttonColour: {
            if ( mode === "victorian" ) return blend( baseColour, victorianInk, 0.04 )
            if ( mode === "light" ) return blend( baseColour, anchorBlack, 0.03 )
            if ( mode === "dark" ) return blend( systemPal.button, anchorBlack, 0.70 )
            return systemPal.button
        }

        readonly property color midColour: {
            if ( mode === "victorian" ) return blend( textColour, baseColour, 0.62 )
            if ( mode === "light" ) return blend( textColour, baseColour, 0.62 )
            if ( mode === "dark" ) return blend( systemPal.mid, anchorBlack, 0.70 )
            return systemPal.mid
        }

        readonly property color windowTextColour: textColour
        readonly property color buttonTextColour: textColour

        readonly property color placeholderTextColour: Qt.rgba( textColour.r, textColour.g, textColour.b, 0.60 )
        readonly property color alternateBaseColour: blend( baseColour, windowColour, 0.5 )

        readonly property color lightColour: blend( baseColour, anchorWhite, 0.28 )
        readonly property color midlightColour: blend( baseColour, anchorWhite, 0.16 )
        readonly property color darkColour: blend( baseColour, anchorBlack, 0.38 )
        readonly property color shadowColour: blend( baseColour, anchorBlack, 0.70 )

        function effectiveMode() {
            if ( mode === "system" ) {
                // Infer a practical light/dark for tuning presets.
                return ( luminance( systemPal.window ) < 0.45 ? "dark" : "light" )
            }

            return mode
        }

        function accentFillForSurface( accentColour, surfaceColour, strength ) {
            // Use a surface-aware fill so bright accents (pink/yellow) do not become
            // glaring pastel blocks in dark mode.
            return blend( surfaceColour, accentColour, strength )
        }

        readonly property bool panelSurfaceIsDark: luminance( panelColour ) < 0.22

        function accentFillStrength( accentColour, role ) {
            // Choose accent fill strength based on the actual surface darkness.
            // This keeps dark surfaces from turning bright accents into pastel glare,
            // while letting light surfaces (Light/Victorian/System-light) use stronger,
            // more intentional accent fills.
            var strength = 0.0

            if ( role === "primary" ) strength = ( panelSurfaceIsDark ? 0.32 : 0.78 )
            if ( role === "secondary" ) strength = ( panelSurfaceIsDark ? 0.30 : 0.72 )
            if ( role === "tertiary" ) strength = ( panelSurfaceIsDark ? 0.24 : 0.60 )

            const aLum = luminance( accentColour )

            if ( panelSurfaceIsDark ) {
                // Lift very dark accents slightly, rein in already bright ones.
                if ( aLum < 0.12 ) strength += 0.06
                if ( aLum > 0.35 ) strength -= 0.04
            } else {
                // Light surfaces: avoid full-bleed glow for very bright accents.
                if ( aLum > 0.72 ) strength -= 0.10
            }

            if ( strength < 0.18 ) strength = 0.18
            if ( strength > 0.92 ) strength = 0.92
            return strength
        }

        function tunedPresetAccent( preset ) {
            if ( typeof oklchUtil === "undefined" ) {
                return preset
            }

            const m = effectiveMode()
            if ( m === "dark" ) {
                // Dark mode: tune by perceptual brightness.
                // The goal is to keep accents cohesive: lift deep accents so they read,
                // and slightly rein in already-bright accents so they do not glare.
                const baseLum = luminance( preset )

                // These constants are chosen empirically to behave well across our preset set.
                // - dL increases as the base colour gets darker
                // - chromaScale decreases slightly for very bright accents (notably yellow)
                var dL = 0.16 - ( 0.20 * baseLum )
                if ( dL < 0.02 ) dL = 0.02
                if ( dL > 0.12 ) dL = 0.12

                var cScale = 1.02 + ( ( 0.55 - baseLum ) * 0.10 )
                if ( cScale < 0.94 ) cScale = 0.94
                if ( cScale > 1.08 ) cScale = 1.08

                return oklchUtil.adjustSrgbInOklchFitted( preset, dL, cScale )
            }

            if ( m === "victorian" ) {
                // Victorian surfaces are bright; keep accents a touch more restrained.
                return oklchUtil.adjustSrgbInOklchFitted( preset, -0.03, 0.92 )
            }

            return preset
        }

        function computedAccentColour() {
            if ( accent === "system" ) {
                return systemPal.highlight
            }

            if ( accent === "victorian" ) {
                return victorianOxblood
            }

            var preset = undefined
            if ( accent === "red" ) preset = presetRed
            if ( accent === "orange" ) preset = presetOrange
            if ( accent === "yellow" ) preset = presetYellow
            if ( accent === "green" ) preset = presetGreen
            if ( accent === "cyan" ) preset = presetCyan
            if ( accent === "blue" ) preset = presetBlue
            if ( accent === "pink" ) preset = presetPink
            if ( accent === "purple" ) preset = presetPurple

            if ( preset !== undefined ) {
                return tunedPresetAccent( preset )
            }

            if ( typeof oklchUtil === "undefined" ) {
                return systemPal.highlight
            }

            if ( accent === "custom" ) {
                if ( appSupervisor && appSupervisor.preferences ) {
                    return oklchUtil.oklchToSrgbFitted(
                        appSupervisor.preferences.customAccentLightness,
                        appSupervisor.preferences.customAccentChroma,
                        appSupervisor.preferences.customAccentHueDegrees )
                }
                return systemPal.highlight
            }

            // Unknown accent value.
            return systemPal.highlight
        }

        readonly property color accentColour: computedAccentColour()
        readonly property color highlightedTextColour: {
            const lum = luminance( accentColour )
            const contrastWithBlack = ( lum + 0.05 ) / 0.05
            const contrastWithWhite = 1.05 / ( lum + 0.05 )
            return ( contrastWithBlack >= contrastWithWhite ? anchorBlack : anchorWhite )
        }

        readonly property color primaryAccentColour: accentColour
        readonly property color secondaryAccentColour: {
            if ( accent === "victorian" ) return victorianNavy
            return blend( accentColour, textColour, 0.42 )
        }

        readonly property color tertiaryAccentColour: {
            if ( accent === "victorian" ) return victorianBrass
            return blend( accentColour, windowColour, 0.72 )
        }

        readonly property color primaryAccentFillColour: {
            return accentFillForSurface( primaryAccentColour, panelColour, accentFillStrength( primaryAccentColour, "primary" ) )
        }

        readonly property color secondaryAccentFillColour: {
            return accentFillForSurface( secondaryAccentColour, panelColour, accentFillStrength( secondaryAccentColour, "secondary" ) )
        }

        readonly property color tertiaryAccentFillColour: {
            return accentFillForSurface( tertiaryAccentColour, panelColour, accentFillStrength( tertiaryAccentColour, "tertiary" ) )
        }

        readonly property color paperColour: windowColour
        readonly property color panelColour: blend( baseColour, mode === "dark" ? anchorWhite : anchorBlack, mode === "dark" ? 0.06 : 0.02 )
        readonly property color panelInsetColour: blend( panelColour, primaryAccentColour, mode === "dark" ? 0.10 : 0.06 )
        readonly property color metaTextColour: blend( textColour, panelColour, 0.25 )
        // Frame tokens.
        // Goal: strong enough to define compartments, but avoid near-black lines on light surfaces.
        readonly property color baseFrameColour: blend( secondaryAccentColour, textColour, 0.28 )
        readonly property color baseFrameInnerColour: blend( baseFrameColour, panelColour, 0.62 )

        // Light surfaces (Light mode + System-light): softer rules.
        readonly property color lightFrameColour: blend( textColour, panelColour, 0.72 )
        readonly property color lightFrameInnerColour: blend( lightFrameColour, panelColour, 0.78 )

        // Victorian mode reads better with less harsh, less near-black rules.
        readonly property color victorianFrameColour: blend( textColour, panelColour, 0.55 )
        readonly property color victorianFrameInnerColour: blend( victorianFrameColour, panelColour, 0.72 )

        readonly property color effectiveFrameColour: {
            if ( effectiveMode() === "victorian" ) return victorianFrameColour
            if ( !panelSurfaceIsDark ) return lightFrameColour
            return baseFrameColour
        }

        readonly property color effectiveFrameInnerColour: {
            if ( effectiveMode() === "victorian" ) return victorianFrameInnerColour
            if ( !panelSurfaceIsDark ) return lightFrameInnerColour
            return baseFrameInnerColour
        }

        readonly property color frameColour: effectiveFrameColour
        readonly property color frameInnerColour: effectiveFrameInnerColour

        // (frame tokens now handled above)
        readonly property color focusRingColour: primaryAccentColour
        readonly property color dangerColour: blend( Qt.rgba( 0.74, 0.18, 0.18, 1 ), textColour, 0.12 )

        readonly property color railPrimaryColour: primaryAccentColour
        readonly property color railSecondaryColour: blend( primaryAccentColour, frameColour, 0.45 )
        readonly property color railTertiaryColour: blend( primaryAccentColour, frameColour, 0.75 )
        readonly property color railDangerColour: dangerColour

        readonly property color linkColour: accentColour
        readonly property color linkVisitedColour: blend( accentColour, textColour, 0.45 )

        readonly property color toolTipBaseColour: baseColour
        readonly property color toolTipTextColour: textColour

        readonly property color softSurface: Qt.rgba( textColour.r, textColour.g, textColour.b, 0.06 )
        readonly property color softerSurface: Qt.rgba( textColour.r, textColour.g, textColour.b, 0.04 )
        readonly property color scrimColour: Qt.rgba( systemPal.shadow.r, systemPal.shadow.g, systemPal.shadow.b, 0.35 )
    }

    palette.window: uiTheme.windowColour
    palette.windowText: uiTheme.windowTextColour
    palette.base: uiTheme.baseColour
    palette.alternateBase: uiTheme.alternateBaseColour
    palette.text: uiTheme.textColour
    palette.button: uiTheme.buttonColour
    palette.buttonText: uiTheme.buttonTextColour
    palette.placeholderText: uiTheme.placeholderTextColour
    palette.light: uiTheme.lightColour
    palette.midlight: uiTheme.midlightColour
    palette.dark: uiTheme.darkColour
    palette.mid: uiTheme.midColour
    palette.shadow: uiTheme.shadowColour
    palette.highlight: uiTheme.accentColour
    palette.highlightedText: uiTheme.highlightedTextColour
    palette.link: uiTheme.linkColour
    palette.linkVisited: uiTheme.linkVisitedColour
    palette.toolTipBase: uiTheme.toolTipBaseColour
    palette.toolTipText: uiTheme.toolTipTextColour

    font.family: uiTheme.bodyFontFamily
    font.pixelSize: uiTheme.fontBasePx

    property bool preferencesOpen: false
    property bool schedulingOpen: false
    property bool supportOpen: false

    function showHelp( targetItem, helpText ) {
        hoverHelp.target = targetItem
        hoverHelp.text = helpText
        hoverHelp.show()
    }

    function hideHelp() {
        hoverHelp.hide()
    }

    function goToLanding() {
        stackView.clear()
        stackView.push( landingComponent )
    }

    function goToShellForJoin() {
        if ( appSupervisor ) {
            appSupervisor.joinRoomEnabled = true
            appSupervisor.hostRoomEnabled = false
        }

        stackView.clear()
        stackView.push( shellComponent )
    }

    function goToShellForHost() {
        if ( appSupervisor ) {
            appSupervisor.hostRoomEnabled = true
            appSupervisor.joinRoomEnabled = false
        }

        stackView.clear()
        stackView.push( shellComponent )
    }

    function openPreferences( categoryIndex ) {
        root.hideHelp()
        root.schedulingOpen = false
        root.supportOpen = false
        var idx = 0
        if ( typeof categoryIndex !== "undefined" && categoryIndex !== null ) {
            var parsed = Number( categoryIndex )
            if ( !isNaN( parsed ) ) {
                idx = Math.max( 0, Math.floor( parsed ) )
            }
        }
        preferencesOverlay.currentCategoryIndex = idx
        preferencesOpen = true
    }

    function closePreferences() {
        root.hideHelp()
        if ( appSupervisor && appSupervisor.preferences ) {
            appSupervisor.preferences.save()
        }

        preferencesOpen = false
    }

    function openScheduling() {
        root.hideHelp()
        root.preferencesOpen = false
        root.supportOpen = false
        root.schedulingOpen = true
    }

    function closeScheduling() {
        root.schedulingOpen = false
    }

    function openSupport() {
        root.hideHelp()
        root.preferencesOpen = false
        root.schedulingOpen = false
        root.supportOpen = true
    }

    function closeSupport() {
        root.supportOpen = false
    }


    Component {
        id: landingComponent

        LandingPage {
            uiMetrics: uiMetrics
            appPalette: root.palette
            theme: uiTheme
            onJoinRequested: {
                root.goToShellForJoin()
            }
            onHostRequested: {
                root.goToShellForHost()
            }
            onPreferencesRequested: {
                root.openPreferences( 0 )
            }
            onSchedulingRequested: {
                root.openScheduling()
            }
            onSupportRequested: {
                root.openSupport()
            }
            onHoverHelpRequested: function( target, text ) {
                root.showHelp( target, text )
            }
            onHoverHelpHideRequested: {
                root.hideHelp()
            }
        }
    }

    Component {
        id: shellComponent

        ShellPage {
            uiMetrics: uiMetrics
            appPalette: root.palette
            theme: uiTheme
            onDisconnectRequested: {
                if ( appSupervisor ) {
                    appSupervisor.joinRoomEnabled = false
                    appSupervisor.hostRoomEnabled = false
                }

                root.goToLanding()
            }
            onPreferencesRequested: {
                root.openPreferences( 0 )
            }
            onSchedulingRequested: {
                root.openScheduling()
            }
            onSupportRequested: {
                root.openSupport()
            }
        }
    }

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: landingComponent

        palette: root.palette
        enabled: !root.preferencesOpen
    }

    Item {
        id: overlayLayer
        anchors.fill: parent
        z: 20000
    }

    HelpTip {
        id: hoverHelp
        overlay: overlayLayer
        z: 15000
        appPalette: root.palette
        theme: uiTheme
    }

    PreferencesOverlay {
        id: preferencesOverlay
        anchors.fill: overlayLayer
        open: root.preferencesOpen
        appPalette: root.palette
        theme: uiTheme

        onCloseRequested: {
            root.closePreferences()
        }
    }

    SchedulingOverlay {
        id: schedulingOverlay
        objectName: "schedulingOverlay"
        anchors.fill: overlayLayer
        open: root.schedulingOpen
        theme: uiTheme
        appPalette: root.palette
        url: "https://elmagnifico.co.uk/schedule/"

        onCloseRequested: root.closeScheduling()
    }

    SupportOverlay {
        id: supportOverlay
        objectName: "supportOverlay"
        anchors.fill: overlayLayer
        open: root.supportOpen
        theme: uiTheme
        appPalette: root.palette
        twitchUrl: "https://twitch.tv/elmagnificogames"
        socialsUrl: "https://elmagnifico.co.uk/socials"

        onCloseRequested: root.closeSupport()
    }

    Component.onCompleted: {
        if ( typeof vcstreamStartupOpenPreferences !== "undefined" ) {
            if ( vcstreamStartupOpenPreferences ) {
                var idx = 0
                if ( typeof vcstreamStartupPreferencesCategoryIndex !== "undefined" ) {
                    idx = vcstreamStartupPreferencesCategoryIndex
                }
                root.openPreferences( idx )
            }
        }
    }
}
