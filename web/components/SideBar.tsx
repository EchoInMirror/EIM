import './SideBar.less'
import React, { useState, useEffect, useMemo, useCallback, useContext, memo } from 'react'
import packets from '../../packets'
import TreeView from '@mui/lab/TreeView'
import TreeItem from '@mui/lab/TreeItem'
import { Resizable } from 're-resizable'
import { actions } from './BottomBar'
import { BottomBarContext } from '../reducer'
import { allowAudioExtensions } from '../utils'
import { Paper, Box, alpha, Toolbar, Button, Tooltip, colors } from '@mui/material'
import {
  FavoriteOutlined, SettingsInputHdmiOutlined, ExpandMore, ChevronRight, GraphicEq, Piano,
  PlaylistAddOutlined, TopicOutlined, StraightenOutlined, CloudOutlined, GraphicEqOutlined
} from '@mui/icons-material'

import type { TreeItemProps } from '@mui/lab/TreeItem'
import type { ButtonProps } from '@mui/material/Button'

type TreeNode = Record<string, boolean>

const DraggableTreeItem: React.FC<TreeItemProps> = props => (
  <TreeItem
    ref={props.draggable
      ? useCallback((elt: Element) => elt?.addEventListener('focusin', e => e.stopImmediatePropagation()), [])
      : props.ref}
    {...props}
  />
)

export interface ItemType {
  title: string
  icon: JSX.Element
  type: packets.ServerboundExplorerData.ExplorerType
  mapNodeProps?: (name: string, tree: TreeNode, path: string) => Partial<TreeItemProps> | undefined
  nodeSort?: (a: JSX.Element, b: JSX.Element) => number
  filter?: (name: string, tree: TreeNode, path: string) => boolean
}
const items: ItemType[] = [
  {
    title: '收藏',
    icon: <FavoriteOutlined />,
    type: packets.ServerboundExplorerData.ExplorerType.FAVORITE
  },
  {
    title: '插件',
    icon: <SettingsInputHdmiOutlined />,
    type: packets.ServerboundExplorerData.ExplorerType.PLUGINS,
    mapNodeProps (name, tree) {
      const [trueName, data] = name.split('#EIM#', 2)
      if (data) {
        const isInstrument = trueName.startsWith('I#')
        const name2 = isInstrument ? trueName.slice(2) : trueName
        const isVST = name2.endsWith(' (VST)')
        return {
          icon: isInstrument ? <Piano /> : <GraphicEq />,
          sx: isVST ? { color: colors.grey[500] } : undefined,
          label: isVST ? <del>{name2}</del> : name2,
          draggable: !tree[name],
          onDragStart: e => {
            window.$dragObject = { type: 'loadPlugin', isInstrument, data: data }
            e.stopPropagation()
          }
        }
      }
    },
    nodeSort: (a, b) => {
      const av = (a.key as string).includes(' (VST)#')
      const bv = (b.key as string).includes(' (VST)#')
      return av === bv ? (a.key as string).localeCompare(b.key as string) : av ? 1 : -1
    }
  },
  {
    title: 'MIDI 序列',
    icon: <StraightenOutlined />,
    type: packets.ServerboundExplorerData.ExplorerType.MIDIs,
    filter (name, tree) {
      return tree[name] || name.endsWith('.mid')
    },
    mapNodeProps (name, tree, data) {
      if (tree[name]) return
      return {
        icon: <StraightenOutlined />,
        draggable: true,
        onDragStart: e => {
          window.$dragObject = { type: 'loadMidi', data: data.replace(/^\//, '') }
          e.stopPropagation()
        }
      }
    }
  },
  {
    title: '工程',
    icon: <TopicOutlined />,
    type: packets.ServerboundExplorerData.ExplorerType.FAVORITE
  },
  {
    title: '预制',
    icon: <PlaylistAddOutlined />,
    type: packets.ServerboundExplorerData.ExplorerType.FAVORITE
  },
  {
    title: '云资源',
    icon: <CloudOutlined />,
    type: packets.ServerboundExplorerData.ExplorerType.FAVORITE
  },
  {
    title: '采样',
    icon: <GraphicEqOutlined />,
    type: packets.ServerboundExplorerData.ExplorerType.SAMPLES,
    filter (name, tree) {
      return tree[name] || allowAudioExtensions.some(it => name.endsWith(it))
    },
    mapNodeProps (name, tree, data) {
      if (tree[name]) return
      return {
        icon: <GraphicEqOutlined />,
        draggable: true,
        onDragStart: e => {
          window.$dragObject = { type: 'loadSample', data: data.replace(/^\//, '') }
          e.stopPropagation()
        }
      }
    }
  }
]

const mapPluginsResult = (data: packets.ClientboundExplorerData) => {
  const result: Record<string, boolean> = {}
  data.folders.sort().forEach(it => (result[it] = true))
  data.files.sort().forEach(it => (result[it] = false))
  return result
}

let callbackMap: Record<string, (() => void) | null> = {}
const Item: React.FC<{ tree: TreeNode, parent: string, type: ItemType }> = ({ type, tree, parent }) => {
  const nodes = []
  const subTrees = useMemo<Record<string, TreeNode>>(() => ({}), [])
  const [, update] = useState(0)
  for (const name in tree) {
    const cur = parent + '/' + name
    if (type.filter && !type.filter(name, tree, cur)) continue
    const children = tree[name] && (subTrees[name] ? <Item tree={subTrees[name]} parent={cur} type={type} /> : <TreeItem nodeId={cur + '$$EMPTY'} />)
    let node: JSX.Element | undefined
    const handleClick = () => {
      if (subTrees[name]) return
      $client.rpc.getExplorerData({ type: type.type, path: cur.replace(/^\//, '') }).then(data => {
        subTrees[name] = mapPluginsResult(data)
        if (callbackMap[cur]) {
          callbackMap[cur]!()
          callbackMap[cur] = null
        }
        update(flag => flag + 1)
      })
    }
    if (type.mapNodeProps) {
      const props = type.mapNodeProps!(name, tree, cur)
      if (props) node = <DraggableTreeItem key={cur} nodeId={cur} label={name} onClick={handleClick} {...props} />
    }
    nodes.push(node || <TreeItem key={cur} nodeId={cur} label={name} onClick={handleClick}>{children}</TreeItem>)
  }
  if (type.nodeSort) nodes.sort(type.nodeSort)
  return (<>{nodes}</>)
}

const Explorer = memo(function Explorer ({ type }: { type: ItemType }) {
  const [tree, setTree] = useState<TreeNode>({})
  const [expanded, setExpanded] = React.useState<string[]>([])
  useEffect(() => {
    callbackMap = {}
    $client.rpc.getExplorerData({ type: type.type, path: '' }).then(mapPluginsResult).then(setTree)
  }, [])
  return (
    <TreeView
      expanded={expanded}
      defaultCollapseIcon={<ExpandMore />}
      defaultExpandIcon={<ChevronRight />}
      sx={{ flexGrow: 1, overflow: 'auto', whiteSpace: 'nowrap' }}
      onNodeToggle={(_, ids: string[]) => setExpanded(ids.filter(it => {
        if (typeof callbackMap[it] === 'undefined') {
          callbackMap[it] = () => setExpanded(prev => [...prev, it])
          return false
        }
        return true
      }))}
    >
      <Item tree={tree} parent='' type={type} />
    </TreeView>
  )
})

const SideBarButton = memo(function SideBarButton ({ active, title, ...props }: { active?: boolean, title: string } & ButtonProps) {
  return (
    <Tooltip title={title} placement='right' arrow>
      <Button
        component='li'
        sx={theme => ({
          color: active ? theme.palette.mode === 'dark' ? theme.palette.text.primary : theme.palette.primary.main : alpha(theme.palette.primary.main, 0.5),
          borderRadius: 0,
          boxShadow: active ? 'inset 2px 0px ' + theme.palette.primary.main : undefined,
          backgroundColor: active ? alpha(theme.palette.primary.main, 0.2) : undefined,
          '&:hover': {
            color: theme.palette.mode === 'dark' ? theme.palette.text.primary : theme.palette.primary.main,
            backgroundColor: active ? alpha(theme.palette.primary.main, 0.2) : 'inherit'
          }
        })}
        {...props as any}
      />
    </Tooltip>
  )
})

// eslint-disable-next-line @typescript-eslint/no-unused-vars, @typescript-eslint/no-empty-function
export let setType = (_val: number) => { }
// eslint-disable-next-line @typescript-eslint/no-unused-vars, @typescript-eslint/no-empty-function
export let setWidth = (_val: number | ((val: number) => number)) => { }

const SideBar = memo(function SideBar () {
  const [width, _setWidth] = useState(0)
  const [type, _setType] = useState(1)
  const [bottomBarType, setBottomBarType] = useContext(BottomBarContext)

  setType = _setType
  setWidth = _setWidth

  return (
    <Box component='nav' className='side-bar' sx={{ zIndex: theme => theme.zIndex.appBar - 1 }}>
      <Paper square elevation={3} sx={{ background: theme => theme.palette.background.bright }}>
        <Toolbar />
        <ul className='types'>
          {items.map((it, i) => (
            <SideBarButton
              key={i}
              title={it.title}
              active={i === type && width as any}
              onClick={() => {
                setType(i)
                if (width) {
                  if (i === type && width) setWidth(0)
                } else setWidth(200)
              }}
            >
              {it.icon}
            </SideBarButton>
          ))}
          <div className='bottom-buttons'>
            {Object.entries(actions).map(([key, it]) => (
              <SideBarButton
                key={key}
                title={it.title}
                active={key === bottomBarType}
                onClick={() => setBottomBarType(key === bottomBarType ? '' : key)}
                onDragOver={() => setBottomBarType(key)}
              >
                {it.icon}
              </SideBarButton>
            ))}
          </div>
        </ul>
      </Paper>
      <Resizable
        onResizeStop={(e, __, ___, d) => (e as MouseEvent).pageX >= 150 && setWidth(width + d.width)}
        onResize={e => (e as MouseEvent).pageX < 150 && setWidth(0)}
        size={{ width } as unknown as { width: number, height: number }}
        minWidth={200}
        maxWidth='60vw'
        enable={{ right: true }}
        handleStyles={{ right: { width: 4, right: -2 } }}
        style={{ display: width ? 'flex' : 'none', flexDirection: 'column' }}
      >
        <Toolbar />
        <Explorer type={items[type]} key={type} />
      </Resizable>
    </Box>
  )
})

export default SideBar
