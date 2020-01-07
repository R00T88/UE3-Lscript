-------------------------------------------------------------------------------
-- PostBeginPlay
-------------------------------------------------------------------------------
function PostBeginPlay()

end

-------------------------------------------------------------------------------
-- Tick
-------------------------------------------------------------------------------
function Tick(DeltaTime, ViewPort)

	if ViewPort == nil then
		return
	end
	
	if ViewPort.Actor == nil then
		return
	end
	
	pPC = ViewPort.Actor

	if pPC ~= nil then
	
	end
	
end

-------------------------------------------------------------------------------
-- PostRender
-------------------------------------------------------------------------------
function PostRender(Canvas)
	
	if Canvas ~= nil then
	
		if pFontSmall ~= nil then
			Canvas.Font = pFontSmall
		end
		
		dwStrLenX, dwStrLenY = Canvas.StrLen('UT3 v2.1 LScript Hack - R00T88', dwStrLenX, dwStrLenY)
		Canvas.SetPos((Canvas.ClipX / 2) - (dwStrLenX / 2), 5)
		Canvas.SetDrawColor(255, 255, 255, 255)
		Canvas.DrawTextClipped('UT3 v2.1 LScript Hack - R00T88', false, 1, 1)
	end	
	
end

-------------------------------------------------------------------------------
-- KeyEvent
-------------------------------------------------------------------------------
function KeyEvent(Key, Action)
	
	if pPC ~= nil then
		
		if Action == 0 then
			if Engine.FNames[Key] == "Insert" then

			end
		end
		
	end
	
	return false
	
end