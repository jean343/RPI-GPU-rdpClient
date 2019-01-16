FIND_PATH( DXGI_INCLUDE dxgi1_2.h
	"C:/Program Files (x86)/Windows Kits/10/Include/10.0.17763.0/shared"
	"C:/Program Files (x86)/Windows Kits/10/Include/10.0.17134.0/shared"
	"C:/Program Files (x86)/Windows Kits/8.1/Include/shared"
	"C:/Program Files (x86)/Windows Kits/8.0/Include/shared"
	"C:/Program Files/Windows Kits/8.1/Include/shared"
	"C:/Program Files/Windows Kits/8.0/Include/shared"
)
FIND_LIBRARY( DXGI_LIBRARY1 d3d11
	"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.17763.0/um/x64"
	"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.17134.0/um/x64"
    "C:/Program Files (x86)/Windows Kits/8.1/Lib/winv6.3/um/x86"
    "C:/Program Files (x86)/Windows Kits/8.0/Lib/winv6.3/um/x86"
    "C:/Program Files/Windows Kits/8.1/Lib/winv6.3/um/x86"
    "C:/Program Files/Windows Kits/8.0/Lib/winv6.3/um/x86"
)
FIND_LIBRARY( DXGI_LIBRARY2 Dxgi
	"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.17763.0/um/x64"
	"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.17134.0/um/x64"
    "C:/Program Files (x86)/Windows Kits/8.1/Lib/winv6.3/um/x86"
    "C:/Program Files (x86)/Windows Kits/8.0/Lib/winv6.3/um/x86"
    "C:/Program Files/Windows Kits/8.1/Lib/winv6.3/um/x86"
    "C:/Program Files/Windows Kits/8.0/Lib/winv6.3/um/x86"
)

if (DXGI_INCLUDE AND DXGI_LIBRARY1 AND DXGI_LIBRARY2 )
	set (DIRECTX_FOUND 1)
	set (DXGI_INCLUDES "${DXGI_INCLUDE}")
	set (DXGI_LIBRARIES "${DXGI_LIBRARY1};${DXGI_LIBRARY2}")
	message("DIRECTX found!")
endif()