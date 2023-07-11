Demonstrating toggleable Windows hit-testing on a transparent Webview2 html page 
for click-through on transparent parts and clickable controls on html page

This requires a host window with WS_LAYERED attribute and a LWA_COLORKEY transparent color.
The same transparent color has to be set to the Webview2 browser as default background color
and the background color of the html body.

A script in the loaded html page informs the host about the location of clickable controls through the web-message mechanism.
Depending on whether the mouse is currently over a control or not, the WS_EX_TRANSPARENT flag is used
to make the whole window clickable or click-through.

For simplicity, this demo uses a WM_TIMER to regularily check the mouse position and adjusting the 
transparency flag which will switch the hit-testing on the whole window on- or off.