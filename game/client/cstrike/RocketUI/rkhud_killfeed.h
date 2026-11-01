#ifndef CSSO_RKHUD_KILLFEED_H
#define CSSO_RKHUD_KILLFEED_H

#include "csso_ui_config.h"

#if defined( USE_CSSOROCKETUI )

#include "hudelement.h"
#include "../../../../rmlui/Include/RmlUi/Core/DataModelHandle.h"

namespace Rml
{
	class ElementDocument;
}

class RkHudKillfeed : public CHudElement
{
public:
	explicit RkHudKillfeed( const char *name );
	virtual ~RkHudKillfeed();

	void LevelInit( void );
	void LevelShutdown( void );
	void SetActive( bool active );
	bool ShouldDraw( void );
	void FireGameEvent( IGameEvent *event );

	// Called by the RocketUI hot-reload callbacks.
	void LoadDocument( void );
	void UnloadDocument( void );

private:
	void ShowPanel( bool show );
	void RetireExpiredEntries( void );

	Rml::ElementDocument *m_pInstance;
	Rml::DataModelHandle m_dataModel;
	bool m_bVisible;
};

#endif // USE_CSSOROCKETUI

#endif // CSSO_RKHUD_KILLFEED_H
