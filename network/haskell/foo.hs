-- sin wave, triangle wave, etc. to be used for the position of the object

sinWave :: Double -> Double
sinWave t = sin ( t * 2.0 * pi )

recWave :: Double -> Double
recWave t
  | m < 0.5 = 1.0
  | otherwise = -1.0              
  where m = t - fromIntegral ( floor t )

triWave :: Double -> Double
triWave t
  | m < 0.5 = 1.0 - m / 0.25
  | otherwise = (-1.0) + ( m - 0.5 ) / 0.25
  where m = t - fromIntegral ( floor t )

-- so for instance, if t is in ms and we want a 1Hz pattern:
-- objectPosition t = triWave ( t / 1000.0 )
