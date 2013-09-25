-- some experiments with random numbers..

-- build some test data to represent how position data would typically be coming to the proxy
-- the object gets ticked at a fixed frequency, for instance every 16ms
-- and every few ticks the position is sent out, say every 4 ticks so once every 64ms
-- then we face a base network latency, some latency jitter and packet loss

-- input for the proxy would be a stream of data packets
-- each packet carries the object's time (as a sequence number probably - to manage packet loss basically)
-- the position of the object at that time
-- the time at which it arrives in the proxy's timeline (subject to base latency and jitter)

import System.Random
import Data.List

-- uses some internal random generator etc.
-- see http://book.realworldhaskell.org/read/monads.html#monads.state.random
rand :: IO Int
rand = getStdRandom (randomR (0, maxBound))

-- produce a list of random values?
-- http://www.haskell.org/haskellwiki/Examples/Random_list
randomlist :: Int -> StdGen -> [Int]
randomlist n = take n . unfoldr (Just . random)

do_spit_list = do
  seed <- newStdGen
  let rs = randomlist 10 seed
  print rs
