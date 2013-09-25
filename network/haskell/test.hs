-- dumb test setup:
-- one snapshot every 4 game frames, so every 64ms
-- constant, stable latency of 10ms (so less than one frame)
-- first: build an input stream manually
-- then: write a function that builds the output stream of positions every 16ms

-- ( object time, object position, proxy time )
data Snapshot = Snapshot Int Double Int deriving ( Show )
snapsData = [ Snapshot 0 0.0 10, Snapshot 64 1.0 74, Snapshot 128 2.0 138 ]
proxyTimeData = [ x * 16 | x <- [1..20] ]

objectTime ( Snapshot t _ _ ) = t
objectPos ( Snapshot _ x _ ) = x
proxyTime ( Snapshot _ _ t ) = t

-- now I want to give a proxy time, and get the position to interpolate from and to
-- uses the last two good positions, whatever they are?
-- lucky they will always be 64 in this case, but what about when latency fluctuates?
-- on average, it'll still be every 64ms anyway (except for cases of packet loss)

-- NOTE: I want a position every 16ms, but if no new data has arrived shouldn't need to extract anything

-- interpolateForTime :: Int -> Maybe Double
-- interpolateForTime t = ?

-- given a time t (proxy time), find the previous two position updates
-- even more simple, find the earliest entry

-- need a predicate to test the proxy time on a snapshot
isEarlierThan :: Int -> Snapshot -> Bool
isEarlierThan t ( Snapshot _ _ proxyTime ) = proxyTime <= t

-- takeWhile, and last element of the list
-- can then use takeWhile and take the last two elements
-- NOTE: unsafe yada yada
findMostRecentForTime :: [ Snapshot ] -> Int -> Snapshot
findMostRecentForTime list time = last $ takeWhile ( isEarlierThan time ) list

-- getting there, need to grab the last two elements of a list safely and complete this

firstTwo :: [a] -> Maybe [a]
firstTwo (x:y:xs) = Just [ x, y ]
firstTwo _ = Nothing

lastTwo :: [a] -> Maybe [a]
lastTwo l
    | len >= 2 = Just $ drop ( len - 2 ) l
    | otherwise = Nothing
    where len = length l

findSnapshotsForTime :: [ Snapshot ] -> Int -> Maybe [ Snapshot ]
findSnapshotsForTime list time = lastTwo $ takeWhile ( isEarlierThan time ) list

-- ok now? given a proxy time (most likely, multiple of 16)
-- find the last two snapshots, and compute the position to interpolate to at that time
-- first version: don't extrapolate, stop moving
--proxyPositionForTime :: [ Snapshot ] -> Int -> Maybe Double

-- match a list of two snapshots, or bail?
--getSnaps :: Maybe [ Snapshot ] -> 
--getSnaps s
--  | otherwise = Nothing
                
-- what things are .. thing is nothing unless I get calls with Just [ s1, s2 ]
-- if I get called with Just [ s1, s2 ] though?
-- MEEEH still missing something .. first I have a function with a Maybe parameter ..
testSnaps :: Maybe [ Snapshot ] -> Bool
testSnaps ( Just _ ) = True
testSnaps Nothing = False

-- NOTE: delta > 0 is an assert really
interpolatePosition :: ( Snapshot, Snapshot ) -> Int -> Double
interpolatePosition ( s1, s2 ) t
    | delta > 0, delta <= 64 = ( fromIntegral delta / 64.0 ) * ( ( objectPos s2 ) - ( objectPos s1 ) ) + ( objectPos s1 )
    | delta > 64 = objectPos s2
    where delta = t - objectTime s2

maybeInterpolate :: Maybe [ Snapshot ] -> Int -> Maybe Double
maybeInterpolate Nothing _ = Nothing
maybeInterpolate ( Just [ s1, s2 ] ) t = Just ( interpolatePosition ( s1, s2 ) t )

-- connect up everything, final signature is:
proxyPositionForTime :: [ Snapshot ] -> Int -> Maybe Double
proxyPositionForTime snaps t = maybeInterpolate ( findSnapshotsForTime snaps t ) t

-- and the test:
test = map ( proxyPositionForTime snapsData ) proxyTimeData
