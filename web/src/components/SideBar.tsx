import './SideBar.less'
import React, { useState, useEffect, useMemo, useCallback } from 'react'
import TreeView from '@mui/lab/TreeView'
import TreeItem from '@mui/lab/TreeItem'
import { Resizable } from 're-resizable'
import { setIsMixer } from './BottomBar'
import { Paper, Box, alpha, Toolbar, Button } from '@mui/material'
import { FavoriteOutlined, SettingsInputHdmiOutlined, ExpandMore, ChevronRight, Piano, Tune, GraphicEq } from '@mui/icons-material'

import type { TreeItemProps } from '@mui/lab/TreeItem'
import type { ButtonProps } from '@mui/material/Button'

// eslint-disable-next-line react/jsx-key
const icons = [<FavoriteOutlined />, <SettingsInputHdmiOutlined />]

type TreeNode = Record<string, boolean>

const DraggableTreeItem: React.FC<TreeItemProps> = props => (
  <TreeItem
    ref={props.draggable
      ? useCallback((elt: Element) => elt?.addEventListener('focusin', e => e.stopImmediatePropagation()), [])
      : props.ref}
    {...props}
  />
)

let callbackMap: Record<string, (() => void) | null> = { }
const Item: React.FC<{ tree: TreeNode, parent: string, type: number }> = ({ type, tree, parent }) => {
  const nodes = []
  const subTrees = useMemo<Record<string, TreeNode>>(() => ({ }), [])
  const [, update] = useState(0)
  for (const name in tree) {
    const cur = parent + '/' + name
    const children = tree[name] && (subTrees[name] ? <Item tree={subTrees[name]} parent={cur} type={type} /> : <TreeItem nodeId={cur + '$$EMPTY'} />)
    let node: JSX.Element | undefined
    const handleClick = () => {
      if (subTrees[name]) return
      $client.getExplorerData(type, cur.replace(/^\//, '')).then(it => {
        subTrees[name] = it
        if (callbackMap[cur]) {
          callbackMap[cur]!()
          callbackMap[cur] = null
        }
        update(flag => flag + 1)
      })
    }
    if (type === 1) {
      const [trueName, data] = name.split('#EIM#', 2)
      if (data) {
        const isInstrument = trueName.startsWith('I#')
        node = (
          <DraggableTreeItem
            key={cur}
            nodeId={cur}
            icon={isInstrument ? <Piano /> : <GraphicEq />}
            label={isInstrument ? trueName.slice(2) : trueName}
            draggable={!tree[name]}
            onDragStart={e => {
              $dragObject = { type: 'loadPlugin', isInstrument, data: data }
              e.stopPropagation()
            }}
            onClick={handleClick}
          >
            {children}
          </DraggableTreeItem>
        )
      }
    }
    nodes.push(node || <TreeItem key={cur} nodeId={cur} label={name} onClick={handleClick}>{children}</TreeItem>)
  }
  return (
    <>
      {nodes}
    </>
  )
}

const Explorer: React.FC<{ type: number }> = ({ type }) => {
  const [tree, setTree] = useState<TreeNode>({})
  const [expanded, setExpanded] = React.useState<string[]>([])
  useEffect(() => {
    callbackMap = { }
    $client.getExplorerData(type, '').then(setTree)
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
}

const SideBarButton: React.FC<{ active?: boolean } & ButtonProps> = ({ active, children, ...props }) => (
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
  >
    {children}
  </Button>
)

const AppBar: React.FC = () => {
  const [width, setWidth] = useState(0)
  const [type, setType] = useState(1)
  const [isMixer, setIsMixer0] = useState(true)
  return (
    <Box component='nav' className='side-bar' sx={{ zIndex: theme => theme.zIndex.appBar - 1 }}>
      <Paper square elevation={3} sx={{ background: theme => theme.palette.background.bright }}>
        <Toolbar />
        <ul className='types'>
          {icons.map((icon, i) => (
            <SideBarButton
              key={i}
              active={i === type && width as any}
              onClick={() => {
                setType(i)
                if (width) {
                  if (i === type && width) setWidth(0)
                } else setWidth(200)
              }}
            >
              {icon}
            </SideBarButton>
          ))}
          <div className='bottom-buttons'>
            <SideBarButton
              active={!isMixer}
              onClick={() => {
                setIsMixer(false)
                setIsMixer0(false)
              }}
            >
              <Piano />
            </SideBarButton>
            <SideBarButton
              active={isMixer}
              onClick={() => {
                setIsMixer(true)
                setIsMixer0(true)
              }}
              onDragOver={() => {
                if (isMixer) return
                setIsMixer(true)
                setIsMixer0(true)
              }}
            >
              <Tune sx={{ transform: 'rotate(90deg)' }} />
            </SideBarButton>
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
        <Explorer type={type} key={type} />
      </Resizable>
    </Box>
  )
}

export default AppBar
