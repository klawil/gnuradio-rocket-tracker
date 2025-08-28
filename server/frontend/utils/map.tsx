import { useEffect, useState } from "react";
import { DeviceState } from "./device";
import { Marker, Popup, TileLayer, useMap } from "react-leaflet";
import { FlightState } from "@/types/Altus";

import L from 'leaflet';
import icon from "leaflet/dist/images/marker-icon.png";
import iconShadow from "leaflet/dist/images/marker-shadow.png";

const DefaultIcon = L.icon({
  iconUrl: icon as unknown as string,
  shadowUrl: iconShadow as unknown as string,
});
L.Marker.prototype.options.icon = DefaultIcon;

const fadeTime = 15 * 60 * 1000;
const flightStateIcons: {
  [key in FlightState]: string;
} = {
  [0]: 'rocket.png',
  [1]: 'rocket.png',
  [2]: 'rocket-pad.png',
  [3]: 'rocket-boost.png',
  [4]: 'rocket-fast.png',
  [5]: 'rocket-coast.png',
  [6]: 'rocket-drogue.png',
  [7]: 'rocket-main.png',
  [8]: 'rocket-landed.png',
  [9]: 'rocket.png',
  [10]: 'rocket.png',
};

export function TileLayers() {
  return <>
    <TileLayer
      maxZoom={18}
      // minZoom={14}
      attribution='Imagery &copy; Esri'
      url='/tiles/{z}-{y}-{x}.png'
    />
    <TileLayer
      maxZoom={20}
      // minZoom={14}
      attribution='Streets &copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a>'
      url='/osm/tile/{z}/{x}/{y}.png'
    />
  </>
}

export function useUserLocation(): [ number, number ] | null {
  const [ loc, setLoc ] = useState<[number, number] | null>(null);

  useEffect(() => {
    navigator.geolocation.watchPosition(
      pos => setLoc([
        pos.coords.latitude,
        pos.coords.longitude,
      ]),
      err => console.error(`Error getting location`, err),
      {
        enableHighAccuracy: true,
      },
    );
  }, []);

  return loc;
}

export function MapMarker({
  device,
}: Readonly<{
  device: DeviceState;
}>) {
  if (!device.Location.Locked) {
    console.log('klawil', 'Did not render device', device);
    return <></>;
  }

  const opacity = device.LastSeen === null || Date.now() - device.LastSeen.getTime() >= fadeTime
    ? 0.5
    : 1;
  const iconPath = flightStateIcons[device.State.Num];

  return <Marker
    opacity={opacity}
    position={[
      device.Location.Latitude,
      device.Location.Longitude,
    ]}
    icon={L.icon({
      iconUrl: `/icons/${iconPath}`,
      iconSize: [ 20, 20 ],
      iconAnchor: [ 10, 10 ],
      popupAnchor: [ 0, -5 ],
    })}
  >
    <Popup>Test</Popup>
  </Marker>;
}

export function CenterDevice({
  center,
  setCenter,
}: Readonly<{
  center: DeviceState | null;
  setCenter: () => void,
}>) {
  const map = useMap();

  useEffect(() => {
    if (center !== null) {
      if (center.Location.Locked) {
        map.setView([
          center.Location.Latitude,
          center.Location.Longitude,
        ])
      }

      setCenter();
    }
  }, [ map, center, setCenter ]);

  return <></>;
}

export function ZoomTwoDevices({
  point1,
  point2,
}: Readonly<{
  point1: [ number, number ],
  point2: [ number, number ],
}>) {
  const map = useMap();

  useEffect(() => {
    if (
      map.getBounds().contains(point1) &&
      map.getBounds().contains(point2)
    ) {
      return;
    }

    map.fitBounds(L.latLngBounds([
      point1,
      point2,
    ]));
  }, [ map, point1, point2, ]);

  return <></>;
}
